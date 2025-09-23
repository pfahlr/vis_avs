#include "avs/audio.hpp"

#include <portaudio.h>
#include <samplerate.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <optional>
#include <string>

#include "avs/audio_portaudio_internal.hpp"
#include "avs/fft.hpp"

namespace avs::portaudio_detail {

CallbackResult processCallbackInput(const float* input, size_t samples, size_t writeIndex,
                                    size_t mask, std::vector<float>& ring) {
  CallbackResult result{writeIndex + samples, input == nullptr};
  for (size_t i = 0; i < samples; ++i) {
    ring[(writeIndex + i) & mask] = input ? input[i] : 0.0f;
  }
  return result;
}

bool callbackIndicatesUnderflow(const void* input, PaStreamCallbackFlags statusFlags) {
  const bool bufferMissing = (input == nullptr);
  const bool portaudioFlagged = (statusFlags & paInputUnderflow) != 0;
  return bufferMissing || portaudioFlagged;
}

StreamNegotiationResult negotiateStream(const StreamNegotiationRequest& request,
                                        const StreamNegotiationDeviceInfo& device,
                                        const FormatSupportQuery& isSupported) {
  StreamNegotiationResult result;
  if (device.maxInputChannels <= 0) {
    return result;
  }

  int desiredChannels = request.requestedChannels.has_value()
                            ? std::max(1, *request.requestedChannels)
                            : std::max(1, request.engineChannels);
  desiredChannels = std::min(desiredChannels, device.maxInputChannels);
  result.channelCount = desiredChannels;

  const double engineRate = static_cast<double>(std::max(1, request.engineSampleRate));
  const double defaultRate = device.defaultSampleRate > 0.0 ? device.defaultSampleRate : engineRate;

  double candidateRate = defaultRate;
  bool candidateIsRequested = false;
  if (request.requestedSampleRate.has_value()) {
    candidateRate = static_cast<double>(*request.requestedSampleRate);
    candidateIsRequested = true;
    if (candidateRate <= 0.0) {
      candidateRate = defaultRate;
      candidateIsRequested = false;
    }
  }

  auto supported = isSupported(result.channelCount, candidateRate);
  if (!supported && candidateIsRequested) {
    double fallback = defaultRate > 0.0 ? defaultRate : engineRate;
    if (fallback != candidateRate) {
      candidateRate = fallback;
      result.usedFallbackRate = true;
      supported = isSupported(result.channelCount, candidateRate);
    }
  }

  if (!supported && candidateRate != engineRate) {
    candidateRate = engineRate;
    if (candidateRate > 0.0) {
      result.usedFallbackRate = true;
      supported = isSupported(result.channelCount, candidateRate);
    }
  }

  result.sampleRate = candidateRate;
  result.supported = supported;
  return result;
}

DeviceSelectionResult resolveInputDeviceIdentifier(const std::optional<std::string>& identifier,
                                                   int deviceCount,
                                                   const std::vector<DeviceSummary>& devices) {
  DeviceSelectionResult result;
  if (!identifier.has_value() || identifier->empty()) {
    return result;
  }

  if (deviceCount <= 0) {
    result.error = "Requested audio input device \"" + *identifier +
                   "\" cannot be satisfied because no capture devices are available.";
    return result;
  }

  auto toLower = [](std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
  };

  const std::string& requested = *identifier;
  bool parsedAsIndex = false;
  int requestedIndex = -1;
  try {
    size_t consumed = 0;
    requestedIndex = std::stoi(requested, &consumed);
    parsedAsIndex = consumed == requested.size();
  } catch (const std::exception&) {
    parsedAsIndex = false;
  }

  auto findSummaryByIndex = [&](int index) -> const DeviceSummary* {
    for (const auto& device : devices) {
      if (device.index == index) {
        return &device;
      }
    }
    return nullptr;
  };

  auto ensureCaptureCapable = [&](const DeviceSummary* summary) -> bool {
    if (summary && summary->maxInputChannels <= 0) {
      result.error = "Requested audio input device \"" + requested +
                     "\" cannot capture audio (no input channels reported).";
      return false;
    }
    return true;
  };

  if (parsedAsIndex) {
    if (requestedIndex < 0 || requestedIndex >= deviceCount) {
      result.error = "Requested audio input device index " + std::to_string(requestedIndex) +
                     " is out of range (0-" + std::to_string(deviceCount - 1) + ").";
      return result;
    }
    const DeviceSummary* summary = findSummaryByIndex(requestedIndex);
    if (!ensureCaptureCapable(summary)) {
      return result;
    }
    result.index = requestedIndex;
    return result;
  }

  std::string needle = toLower(requested);
  for (const auto& device : devices) {
    std::string lowered = toLower(device.name);
    if (lowered.find(needle) != std::string::npos) {
      if (!ensureCaptureCapable(&device)) {
        return result;
      }
      result.index = device.index;
      return result;
    }
  }

  result.error = "Requested audio input device \"" + requested +
                 "\" was not found. Use --list-input-devices to inspect available capture "
                 "endpoints.";
  return result;
}

}  // namespace avs::portaudio_detail

namespace avs {

struct AudioInput::Impl {
  static constexpr int kFftSize = 2048;
  static constexpr float kBandSmooth = 0.2f;  // IIR smoothing coefficient

  explicit Impl(const AudioInputConfig& config)
      : engineSampleRate(std::max(1, config.engineSampleRate)),
        engineChannels(std::max(1, config.engineChannels)),
        sampleRate(engineSampleRate),
        channels(engineChannels),
        fft(kFftSize) {
    const size_t rbSize = 1 << 16;  // power-of-two ring buffer (floats)
    ring.resize(rbSize);
    mask = rbSize - 1;
    mono.resize(kFftSize);
    spectrum.resize(kFftSize / 2);
    inputSampleRate = static_cast<double>(sampleRate);

    if (Pa_Initialize() != paNoError) {
      return;
    }

    PaStreamParameters params{};
    params.sampleFormat = paFloat32;
    params.hostApiSpecificStreamInfo = nullptr;

    const PaDeviceInfo* info = nullptr;

    if (config.requestedDeviceIdentifier && !config.requestedDeviceIdentifier->empty()) {
      const std::string& identifier = *config.requestedDeviceIdentifier;
      PaDeviceIndex deviceCount = Pa_GetDeviceCount();
      if (deviceCount < 0) {
        std::fprintf(stderr, "Failed to enumerate PortAudio devices: %s\n",
                     Pa_GetErrorText(deviceCount));
        return;
      }

      std::vector<portaudio_detail::DeviceSummary> devices;
      devices.reserve(deviceCount > 0 ? static_cast<size_t>(deviceCount) : 0);
      for (PaDeviceIndex i = 0; i < deviceCount; ++i) {
        if (const PaDeviceInfo* candidate = Pa_GetDeviceInfo(i)) {
          portaudio_detail::DeviceSummary summary;
          summary.index = static_cast<int>(i);
          summary.name = candidate->name ? candidate->name : "";
          summary.maxInputChannels = candidate->maxInputChannels;
          devices.push_back(std::move(summary));
        }
      }

      auto selection = portaudio_detail::resolveInputDeviceIdentifier(
          config.requestedDeviceIdentifier, static_cast<int>(deviceCount), devices);
      if (!selection.error.empty()) {
        std::fprintf(stderr, "%s\n", selection.error.c_str());
        return;
      }
      if (!selection.index.has_value()) {
        std::fprintf(stderr, "Requested audio input device \"%s\" could not be resolved.\n",
                     identifier.c_str());
        return;
      }

      params.device = static_cast<PaDeviceIndex>(*selection.index);
      info = Pa_GetDeviceInfo(params.device);
      if (!info) {
        std::fprintf(stderr, "PortAudio did not provide information for input device %d.\n",
                     static_cast<int>(params.device));
        return;
      }
      if (info->maxInputChannels <= 0) {
        std::fprintf(stderr,
                     "Requested audio input device \"%s\" cannot capture audio (no input channels "
                     "reported).\n",
                     identifier.c_str());
        return;
      }
    } else {
      params.device = Pa_GetDefaultInputDevice();
      if (params.device == paNoDevice) {
        std::fprintf(stderr, "No default PortAudio input device is available.\n");
        return;
      }
      info = Pa_GetDeviceInfo(params.device);
      if (!info) {
        std::fprintf(stderr,
                     "PortAudio did not provide information for the default input device.\n");
        return;
      }
      if (info->maxInputChannels <= 0) {
        std::fprintf(stderr,
                     "Default PortAudio input device cannot capture audio (no input channels "
                     "reported).\n");
        return;
      }
    }

    params.suggestedLatency = info->defaultLowInputLatency;

    portaudio_detail::StreamNegotiationRequest request;
    request.engineSampleRate = engineSampleRate;
    request.engineChannels = engineChannels;
    request.requestedSampleRate = config.requestedSampleRate;
    request.requestedChannels = config.requestedChannels;

    portaudio_detail::StreamNegotiationDeviceInfo deviceInfo{info->defaultSampleRate,
                                                             info->maxInputChannels};

    auto negotiation =
        portaudio_detail::negotiateStream(request, deviceInfo, [&](int ch, double rate) {
          params.channelCount = ch;
          return Pa_IsFormatSupported(&params, nullptr, rate) == paNoError;
        });

    if (!negotiation.supported) {
      return;
    }

    params.channelCount = negotiation.channelCount;
    channels = negotiation.channelCount;
    inputSampleRate = negotiation.sampleRate;

    if (negotiation.usedFallbackRate && config.requestedSampleRate.has_value()) {
      std::fprintf(stderr, "Requested sample rate %d Hz not supported; using %.0f Hz instead.\n",
                   *config.requestedSampleRate, negotiation.sampleRate);
    }

    auto err =
        Pa_OpenStream(&stream, &params, nullptr, negotiation.sampleRate,
                      paFramesPerBufferUnspecified, paNoFlag, &AudioInput::Impl::paCallback, this);
    if (err != paNoError) {
      stream = nullptr;
      return;
    }

    if (const PaStreamInfo* streamInfo = Pa_GetStreamInfo(stream)) {
      inputSampleRate = streamInfo->sampleRate;
    }

    if (std::abs(inputSampleRate - static_cast<double>(engineSampleRate)) > 1e-3) {
      useResampler = true;
      resampleRatio = static_cast<double>(engineSampleRate) / inputSampleRate;
      int resErr = 0;
      resampler = src_new(SRC_SINC_FASTEST, channels, &resErr);
      if (!resampler || resErr != 0) {
        if (resampler) {
          src_delete(resampler);
          resampler = nullptr;
        }
        Pa_CloseStream(stream);
        stream = nullptr;
        return;
      }
      sampleRate = engineSampleRate;
    } else {
      sampleRate = static_cast<int>(std::lround(inputSampleRate));
    }

    if (Pa_StartStream(stream) != paNoError) {
      Pa_CloseStream(stream);
      stream = nullptr;
      return;
    }

    ok = true;
  }

  ~Impl() {
    if (stream) {
      Pa_StopStream(stream);
      Pa_CloseStream(stream);
    }
    if (resampler) {
      src_delete(resampler);
      resampler = nullptr;
    }
    Pa_Terminate();
  }

  static int paCallback(const void* input, void*, unsigned long frameCount,
                        const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags statusFlags,
                        void* userData) {
    auto* self = static_cast<Impl*>(userData);
    const float* in = static_cast<const float*>(input);
    size_t w = self->writeIndex.load(std::memory_order_relaxed);
    const bool underflowFlagged = portaudio_detail::callbackIndicatesUnderflow(in, statusFlags);

    portaudio_detail::CallbackResult result{w, in == nullptr};
    if (self->useResampler) {
      const double ratio = self->resampleRatio;
      size_t outFrames = static_cast<size_t>(std::ceil(static_cast<double>(frameCount) * ratio));
      if (outFrames == 0) {
        outFrames = 1;
      }
      if (in == nullptr) {
        result = portaudio_detail::processCallbackInput(
            nullptr, outFrames * static_cast<size_t>(self->channels), w, self->mask, self->ring);
      } else {
        const size_t requiredFrames = outFrames + 8;
        const size_t requiredSamples = requiredFrames * static_cast<size_t>(self->channels);
        if (self->resampleBuffer.size() < requiredSamples) {
          self->resampleBuffer.resize(requiredSamples);
        }
        SRC_DATA data{};
        data.data_in = in;
        data.input_frames = static_cast<long>(frameCount);
        data.data_out = self->resampleBuffer.data();
        data.output_frames = static_cast<long>(requiredFrames);
        data.end_of_input = 0;
        data.src_ratio = ratio;
        const int err = src_process(self->resampler, &data);
        if (err != 0) {
          self->resampleFailed.store(true, std::memory_order_relaxed);
          return paAbort;
        }
        const size_t producedSamples =
            static_cast<size_t>(data.output_frames_gen) * static_cast<size_t>(self->channels);
        result = portaudio_detail::processCallbackInput(self->resampleBuffer.data(),
                                                        producedSamples, w, self->mask, self->ring);
      }
    } else {
      const size_t samples = static_cast<size_t>(frameCount) * static_cast<size_t>(self->channels);
      result = portaudio_detail::processCallbackInput(in, samples, w, self->mask, self->ring);
    }

    if (underflowFlagged || result.underflow) {
      self->inputUnderflowCount.fetch_add(1, std::memory_order_relaxed);
    }
    self->writeIndex.store(result.nextWriteIndex, std::memory_order_release);
    return paContinue;
  }

  AudioState poll() {
    AudioState state;
    if (!ok) {
      return state;
    }

    if (resampleFailed.load(std::memory_order_acquire)) {
      reportResampleFailure();
      return state;
    }

    state.sampleRate = sampleRate;
    state.inputSampleRate = static_cast<int>(std::lround(inputSampleRate));
    state.channels = channels;

    const auto underflows = inputUnderflowCount.load(std::memory_order_acquire);
    if (underflows > lastUnderflowCount) {
      ++consecutiveUnderflowPolls;
      lastUnderflowCount = underflows;
    } else {
      consecutiveUnderflowPolls = 0;
    }

    if (consecutiveUnderflowPolls >= kMaxConsecutiveUnderflows) {
      reportUnderflow();
      return state;
    }
    const size_t needed = kFftSize * static_cast<size_t>(channels);
    const size_t w = writeIndex.load(std::memory_order_acquire);
    if (w < needed) {
      return state;  // not enough data yet
    }
    size_t start = w - needed;
    for (int i = 0; i < kFftSize; ++i) {
      float sum = 0.0f;
      for (int c = 0; c < channels; ++c) {
        sum += ring[(start + i * channels + c) & mask];
      }
      mono[i] = sum / static_cast<float>(channels);
    }
    float sumSq = 0.0f;
    for (float v : mono) {
      sumSq += v * v;
    }
    state.rms = std::sqrt(sumSq / kFftSize);

    fft.compute(mono.data(), spectrum);
    state.spectrum = spectrum;

    AudioState::LegacyBuffer specLegacy{};
    specLegacy.fill(0.0f);
    const size_t legacyCount = AudioState::kLegacyVisSamples;
    const size_t spectrumSize = spectrum.size();
    if (legacyCount > 0 && spectrumSize > 0) {
      const double scale = static_cast<double>(spectrumSize) / static_cast<double>(legacyCount);
      for (size_t i = 0; i < legacyCount; ++i) {
        size_t begin = static_cast<size_t>(std::floor(static_cast<double>(i) * scale));
        size_t end = static_cast<size_t>(std::floor(static_cast<double>(i + 1) * scale));
        if (end <= begin) end = std::min(begin + 1, spectrumSize);
        double sum = 0.0;
        size_t count = 0;
        for (size_t j = begin; j < end && j < spectrumSize; ++j) {
          sum += spectrum[j];
          ++count;
        }
        float value = 0.0f;
        if (count > 0) {
          value = static_cast<float>(sum / static_cast<double>(count));
        } else if (begin < spectrumSize) {
          value = spectrum[begin];
        }
        specLegacy[i] = value;
      }
    }
    state.spectrumLegacy[0] = specLegacy;
    state.spectrumLegacy[1] = specLegacy;

    for (auto& scope : state.oscilloscope) {
      scope.fill(0.0f);
    }
    const size_t legacySamples = AudioState::kLegacyVisSamples;
    const size_t channelCount = static_cast<size_t>(channels);
    if (legacySamples > 0 && channelCount > 0) {
      const size_t sampleStart =
          kFftSize > legacySamples ? static_cast<size_t>(kFftSize) - legacySamples : 0;
      for (int ch = 0; ch < std::min<int>(channels, 2); ++ch) {
        auto& dest = state.oscilloscope[static_cast<size_t>(ch)];
        for (size_t i = 0; i < legacySamples; ++i) {
          size_t sampleIndex = sampleStart + i;
          if (sampleIndex >= static_cast<size_t>(kFftSize)) break;
          size_t idx = (start + sampleIndex * channelCount + static_cast<size_t>(ch)) & mask;
          dest[i] = ring[idx];
        }
      }
      if (channels == 1) {
        state.oscilloscope[1] = state.oscilloscope[0];
      }
    }

    std::array<float, 3> newBands{0.f, 0.f, 0.f};
    std::array<int, 3> counts{0, 0, 0};
    const double binHz = static_cast<double>(sampleRate) / kFftSize;
    for (size_t i = 0; i < spectrum.size(); ++i) {
      double freq = i * binHz;
      float mag = spectrum[i];
      if (freq < 250.0) {
        newBands[0] += mag;
        counts[0]++;
      } else if (freq < 4000.0) {
        newBands[1] += mag;
        counts[1]++;
      } else {
        newBands[2] += mag;
        counts[2]++;
      }
    }
    for (int i = 0; i < 3; ++i) {
      if (counts[i] > 0) {
        newBands[i] /= static_cast<float>(counts[i]);
      }
      bands[i] = bands[i] * (1.0f - kBandSmooth) + newBands[i] * kBandSmooth;
    }
    state.bands = bands;
    state.timeSeconds = Pa_GetStreamTime(stream);
    return state;
  }

  void reportUnderflow() {
    if (underflowReported) {
      return;
    }
    underflowReported = true;
    stopStream();
    ok = false;
    const auto underflows = inputUnderflowCount.load(std::memory_order_acquire);
    std::fprintf(stderr,
                 "PortAudio repeatedly reported input underflow (observed %u events). Capture has "
                 "been stopped to avoid feeding silent audio. Please verify your capture device "
                 "configuration.\n",
                 static_cast<unsigned>(underflows));
  }

  void reportResampleFailure() {
    if (resampleErrorReported) {
      return;
    }
    resampleErrorReported = true;
    stopStream();
    ok = false;
    std::fprintf(stderr,
                 "Audio resampler failure detected; capture has been stopped. Please verify your "
                 "audio configuration.\n");
  }

  void stopStream() {
    if (stream) {
      Pa_StopStream(stream);
      Pa_CloseStream(stream);
      stream = nullptr;
    }
  }

  static constexpr int kMaxConsecutiveUnderflows = 3;

  int engineSampleRate;
  int engineChannels;
  int sampleRate;
  int channels;
  double inputSampleRate = 0.0;
  bool useResampler = false;
  double resampleRatio = 1.0;
  SRC_STATE* resampler = nullptr;
  std::vector<float> resampleBuffer;
  FFT fft;
  std::vector<float> ring;
  size_t mask = 0;
  std::atomic<size_t> writeIndex{0};
  std::vector<float> mono;
  std::vector<float> spectrum;
  std::array<float, 3> bands{{0.f, 0.f, 0.f}};
  PaStream* stream = nullptr;
  bool ok = false;
  std::atomic<uint32_t> inputUnderflowCount{0};
  uint32_t lastUnderflowCount = 0;
  int consecutiveUnderflowPolls = 0;
  bool underflowReported = false;
  std::atomic<bool> resampleFailed{false};
  bool resampleErrorReported = false;
};

AudioInput::AudioInput(const AudioInputConfig& config)
    : impl_(new Impl(config)), ok_(impl_ && impl_->ok) {}

AudioInput::~AudioInput() { delete impl_; }

AudioState AudioInput::poll() {
  if (!impl_) {
    return AudioState{};
  }
  auto state = impl_->poll();
  if (impl_ && !impl_->ok) {
    ok_ = false;
  }
  return state;
}

}  // namespace avs
