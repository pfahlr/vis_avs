#include "avs/audio.hpp"

#include <portaudio.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>

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

}  // namespace avs::portaudio_detail

namespace avs {

struct AudioInput::Impl {
  static constexpr int kFftSize = 2048;
  static constexpr float kBandSmooth = 0.2f;  // IIR smoothing coefficient

  Impl(int sampleRate, int channels) : sampleRate(sampleRate), channels(channels), fft(kFftSize) {
    const size_t rbSize = 1 << 16;  // power-of-two ring buffer (floats)
    ring.resize(rbSize);
    mask = rbSize - 1;
    mono.resize(kFftSize);

    if (Pa_Initialize() != paNoError) {
      return;
    }
    PaStreamParameters params;
    params.device = Pa_GetDefaultInputDevice();
    if (params.device == paNoDevice) {
      return;
    }
    params.channelCount = channels;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = Pa_GetDeviceInfo(params.device)->defaultLowInputLatency;
    params.hostApiSpecificStreamInfo = nullptr;

    auto err = Pa_OpenStream(&stream, &params, nullptr, sampleRate, paFramesPerBufferUnspecified,
                             paNoFlag, &AudioInput::Impl::paCallback, this);
    if (err != paNoError) {
      return;
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
    Pa_Terminate();
  }

  static int paCallback(const void* input, void*, unsigned long frameCount,
                        const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags statusFlags,
                        void* userData) {
    auto* self = static_cast<Impl*>(userData);
    const float* in = static_cast<const float*>(input);
    const size_t samples = frameCount * self->channels;
    size_t w = self->writeIndex.load(std::memory_order_relaxed);
    const bool underflowFlagged = (statusFlags & paInputUnderflow) != 0;
    const auto result =
        portaudio_detail::processCallbackInput(in, samples, w, self->mask, self->ring);
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

  int sampleRate;
  int channels;
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

  static constexpr int kMaxConsecutiveUnderflows = 3;

  void reportUnderflow() {
    if (underflowReported) {
      return;
    }
    underflowReported = true;
    if (stream) {
      Pa_StopStream(stream);
      Pa_CloseStream(stream);
      stream = nullptr;
    }
    ok = false;
    std::fprintf(stderr,
                 "PortAudio input underflow detected; capture has been stopped. Please verify your "
                 "audio configuration.\n");
  }
};

AudioInput::AudioInput(int sampleRate, int channels)
    : impl_(new Impl(sampleRate, channels)), ok_(impl_ && impl_->ok) {}

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
