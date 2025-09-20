#include "avs/eel.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

namespace {
std::array<std::vector<double>, avs::EelVm::kMegaBufBlocks> gMegaBlocks;
std::mutex gMegaMutex;
EEL_F gMegaError = 0.0;
}  // namespace

namespace avs {

extern "C" void NSEEL_HOSTSTUB_EnterMutex() {}
extern "C" void NSEEL_HOSTSTUB_LeaveMutex() {}

EelVm::EelVm() {
  static bool init = false;
  if (!init) {
    NSEEL_init();
    init = true;
  }
  ctx_ = NSEEL_VM_alloc();
  rng_.seed(0);
  NSEEL_VM_SetCustomFuncThis(ctx_, this);
  NSEEL_addfunc_retval("rand", 0, NSEEL_PProc_THIS, (void*)funcRand);
  NSEEL_addfunc_retval("clamp", 3, NSEEL_PProc_THIS, (void*)funcClamp);
  NSEEL_addfunc_retval("smooth", 3, NSEEL_PProc_THIS, (void*)funcSmooth);
  NSEEL_addfunc_retval("getosc", 3, NSEEL_PProc_THIS, (void*)funcGetOsc);
  NSEEL_addfunc_retval("getspec", 3, NSEEL_PProc_THIS, (void*)funcGetSpec);
  NSEEL_addfunc_retval("gettime", 1, NSEEL_PProc_THIS, (void*)funcGetTime);
  NSEEL_addfunc_retval("getkbmouse", 1, NSEEL_PProc_THIS, (void*)funcGetKbMouse);
  NSEEL_addfunc_retval("setmousepos", 2, NSEEL_PProc_THIS, (void*)funcSetMousePos);
  NSEEL_addfunc_retptr("megabuf", 1, NSEEL_PProc_THIS, (void*)funcMegaBuf);
  NSEEL_addfunc_retptr("gmegabuf", 1, NSEEL_PProc_THIS, (void*)funcGMegaBuf);
}

EelVm::~EelVm() {
  if (ctx_) NSEEL_VM_free(ctx_);
}

EEL_F* EelVm::regVar(const char* name) { return NSEEL_VM_regvar(ctx_, name); }

NSEEL_CODEHANDLE EelVm::compile(const std::string& code) {
  return NSEEL_code_compile(ctx_, code.c_str(), 0);
}

void EelVm::execute(NSEEL_CODEHANDLE code) { NSEEL_code_execute(code); }

void EelVm::freeCode(NSEEL_CODEHANDLE code) { NSEEL_code_free(code); }

void EelVm::setLegacySources(const LegacySources& sources) {
  legacySources_ = sources;
  if (legacySources_.sampleCount > kLegacyVisSamples) {
    legacySources_.sampleCount = kLegacyVisSamples;
  }
  legacySources_.channels = std::clamp(legacySources_.channels, 0, 2);
}

EEL_F NSEEL_CGEN_CALL EelVm::funcRand(void* opaque) {
  auto* self = static_cast<EelVm*>(opaque);
  return static_cast<double>(self->rng_()) / static_cast<double>(self->rng_.max());
}

EEL_F NSEEL_CGEN_CALL EelVm::funcClamp(void* opaque, EEL_F* x, EEL_F* lo, EEL_F* hi) {
  (void)opaque;
  EEL_F v = *x;
  if (v < *lo) v = *lo;
  if (v > *hi) v = *hi;
  return v;
}

EEL_F NSEEL_CGEN_CALL EelVm::funcSmooth(void* opaque, EEL_F* prev, EEL_F* x, EEL_F* a) {
  (void)opaque;
  return *prev + (*x - *prev) * (*a);
}

EEL_F NSEEL_CGEN_CALL EelVm::funcGetOsc(void* opaque, EEL_F* band, EEL_F* bandw, EEL_F* chan) {
  auto* self = static_cast<EelVm*>(opaque);
  if (!self->legacySources_.oscBase || self->legacySources_.sampleCount == 0) {
    return 0.0;
  }
  const double b = band ? *band : 0.0;
  const double w = bandw ? *bandw : 0.0;
  const int channel = chan ? static_cast<int>(*chan + 0.5) : 0;
  return self->computeVisSample(self->legacySources_.oscBase,
                                self->legacySources_.sampleCount,
                                128,
                                channel,
                                b,
                                w);
}

EEL_F NSEEL_CGEN_CALL EelVm::funcGetSpec(void* opaque, EEL_F* band, EEL_F* bandw, EEL_F* chan) {
  auto* self = static_cast<EelVm*>(opaque);
  if (!self->legacySources_.specBase || self->legacySources_.sampleCount == 0) {
    return 0.0;
  }
  const double b = band ? *band : 0.0;
  const double w = bandw ? *bandw : 0.0;
  const int channel = chan ? static_cast<int>(*chan + 0.5) : 0;
  return 0.5 * self->computeVisSample(self->legacySources_.specBase,
                                      self->legacySources_.sampleCount,
                                      0,
                                      channel,
                                      b,
                                      w);
}

EEL_F NSEEL_CGEN_CALL EelVm::funcGetTime(void* opaque, EEL_F* sc) {
  auto* self = static_cast<EelVm*>(opaque);
  const double arg = sc ? *sc : 0.0;
  if (std::abs(arg + 1.0) < 0.001) {
    return self->legacySources_.audioTimeSeconds;
  }
  if (std::abs(arg + 2.0) < 0.001) {
    return self->legacySources_.audioTimeSeconds * 1000.0;
  }
  return self->legacySources_.engineTimeSeconds - arg;
}

EEL_F NSEEL_CGEN_CALL EelVm::funcGetKbMouse(void* opaque, EEL_F* which) {
  auto* self = static_cast<EelVm*>(opaque);
  const int w = which ? static_cast<int>(*which + 0.5) : 0;
  const auto& mouse = self->legacySources_.mouse;
  switch (w) {
    case 1:
      return mouse.normX;
    case 2:
      return mouse.normY;
    case 3:
      return mouse.left ? 1.0 : 0.0;
    case 4:
      return mouse.right ? 1.0 : 0.0;
    case 5:
      return mouse.middle ? 1.0 : 0.0;
    default:
      break;
  }
  if (w > 5) return 0.0;
  return 0.0;
}

EEL_F NSEEL_CGEN_CALL EelVm::funcSetMousePos(void* opaque, EEL_F* x, EEL_F* y) {
  (void)opaque;
  (void)x;
  (void)y;
  return 0.0;
}

EEL_F_PTR NSEEL_CGEN_CALL EelVm::funcMegaBuf(void* opaque, EEL_F* which) {
  auto* self = static_cast<EelVm*>(opaque);
  const int index = which ? static_cast<int>(*which + 0.0001) : 0;
  return self->getMegaBufEntry(index);
}

EEL_F_PTR NSEEL_CGEN_CALL EelVm::funcGMegaBuf(void* opaque, EEL_F* which) {
  (void)opaque;
  const int index = which ? static_cast<int>(*which + 0.0001) : 0;
  return getGlobalMegaBufEntry(index);
}

EEL_F EelVm::computeVisSample(const std::uint8_t* base,
                               size_t sampleCount,
                               int xorv,
                               int channelRequest,
                               double band,
                               double bandw) const {
  if (!base || sampleCount == 0) {
    return 0.0;
  }
  if (channelRequest && channelRequest != 1 && channelRequest != 2) {
    return 0.0;
  }
  const int count = static_cast<int>(sampleCount);
  int bc = static_cast<int>(band * static_cast<double>(count));
  int bw = static_cast<int>(bandw * static_cast<double>(count));
  if (bw < 1) bw = 1;
  bc -= bw / 2;
  if (bc < 0) {
    bw += bc;
    bc = 0;
  }
  if (bc > count - 1) bc = count - 1;
  if (bc + bw > count) bw = count - bc;
  if (bw <= 0) {
    return 0.0;
  }
  const std::uint8_t* ch0 = base;
  const std::uint8_t* ch1 = legacySources_.channels > 1 ? base + sampleCount : nullptr;

  if (channelRequest == 0) {
    double accum = 0.0;
    const double denom = (legacySources_.channels > 1 ? 255.0 : 127.5) * static_cast<double>(bw);
    if (!denom) return 0.0;
    for (int x = 0; x < bw; ++x) {
      accum += static_cast<double>((ch0[bc + x] ^ xorv) - xorv);
      if (legacySources_.channels > 1 && ch1) {
        accum += static_cast<double>((ch1[bc + x] ^ xorv) - xorv);
      } else if (legacySources_.channels <= 1 && xorv != 0) {
        accum += static_cast<double>((ch0[bc + x] ^ xorv) - xorv);
      }
    }
    return accum / denom;
  }

  const std::uint8_t* src = channelRequest == 2 ? ch1 : ch0;
  if (!src) {
    return 0.0;
  }
  double accum = 0.0;
  for (int x = 0; x < bw; ++x) {
    accum += static_cast<double>((src[bc + x] ^ xorv) - xorv);
  }
  const double denom = 127.5 * static_cast<double>(bw);
  return denom != 0.0 ? accum / denom : 0.0;
}

EEL_F_PTR EelVm::getMegaBufEntry(int index) {
  if (index < 0) {
    return &megaError_;
  }
  const int block = index / kMegaBufItemsPerBlock;
  if (block < 0 || block >= kMegaBufBlocks) {
    return &megaError_;
  }
  const int entry = index % kMegaBufItemsPerBlock;
  auto& blk = megaBlocks_[static_cast<size_t>(block)];
  if (blk.empty()) {
    blk.assign(kMegaBufItemsPerBlock, 0.0);
  }
  return blk.data() + static_cast<size_t>(entry);
}

EEL_F_PTR EelVm::getGlobalMegaBufEntry(int index) {
  if (index < 0) {
    return &gMegaError;
  }
  const int block = index / kMegaBufItemsPerBlock;
  if (block < 0 || block >= kMegaBufBlocks) {
    return &gMegaError;
  }
  const int entry = index % kMegaBufItemsPerBlock;
  std::lock_guard<std::mutex> guard(gMegaMutex);
  auto& blk = gMegaBlocks[static_cast<size_t>(block)];
  if (blk.empty()) {
    blk.assign(avs::EelVm::kMegaBufItemsPerBlock, 0.0);
  }
  return blk.data() + static_cast<size_t>(entry);
}

}  // namespace avs
