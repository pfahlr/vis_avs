#pragma once

/** \file
 *  \brief Simple wrapper around the NS-EEL virtual machine.
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "ns-eel-addfuncs.h"
#include "ns-eel.h"

namespace avs {

struct MouseState {
  double normX = 0.0;
  double normY = 0.0;
  bool left = false;
  bool right = false;
  bool middle = false;
};

/** \brief Lightweight RAII wrapper for an NS-EEL VM.
 */
class EelVm {
 public:
  static constexpr size_t kLegacyVisSamples = 576;
  static constexpr int kMegaBufBlocks = 64;
  static constexpr int kMegaBufItemsPerBlock = 16384;

  struct LegacySources {
    const std::uint8_t* oscBase = nullptr;
    const std::uint8_t* specBase = nullptr;
    size_t sampleCount = 0;
    int channels = 0;
    double audioTimeSeconds = 0.0;
    double engineTimeSeconds = 0.0;
    MouseState mouse{};
  };

  EelVm();
  ~EelVm();

  /** Register a variable exposed to scripts. */
  EEL_F* regVar(const char* name);

  /** Compile a script and return a handle. */
  NSEEL_CODEHANDLE compile(const std::string& code);

  /** Execute previously compiled code. */
  void execute(NSEEL_CODEHANDLE code);

  /** Free a compiled script. */
  void freeCode(NSEEL_CODEHANDLE code);

  /** Update legacy Winamp compatibility data used by VM callbacks. */
  void setLegacySources(const LegacySources& sources);

 private:
  static EEL_F NSEEL_CGEN_CALL funcRand(void* opaque);
  static EEL_F NSEEL_CGEN_CALL funcClamp(void* opaque, EEL_F* x, EEL_F* lo, EEL_F* hi);
  static EEL_F NSEEL_CGEN_CALL funcSmooth(void* opaque, EEL_F* prev, EEL_F* x, EEL_F* a);
  static EEL_F NSEEL_CGEN_CALL funcGetOsc(void* opaque, EEL_F* band, EEL_F* bandw, EEL_F* chan);
  static EEL_F NSEEL_CGEN_CALL funcGetSpec(void* opaque, EEL_F* band, EEL_F* bandw, EEL_F* chan);
  static EEL_F NSEEL_CGEN_CALL funcGetTime(void* opaque, EEL_F* sc);
  static EEL_F NSEEL_CGEN_CALL funcGetKbMouse(void* opaque, EEL_F* which);
  static EEL_F NSEEL_CGEN_CALL funcSetMousePos(void* opaque, EEL_F* x, EEL_F* y);
  static EEL_F_PTR NSEEL_CGEN_CALL funcMegaBuf(void* opaque, EEL_F* which);
  static EEL_F_PTR NSEEL_CGEN_CALL funcGMegaBuf(void* opaque, EEL_F* which);

  EEL_F computeVisSample(const std::uint8_t* base,
                         size_t sampleCount,
                         int xorv,
                         int channelRequest,
                         double band,
                         double bandw) const;
  EEL_F_PTR getMegaBufEntry(int index);
  static EEL_F_PTR getGlobalMegaBufEntry(int index);

  NSEEL_VMCTX ctx_{};
  std::mt19937 rng_{};
  LegacySources legacySources_{};
  std::array<std::vector<double>, kMegaBufBlocks> megaBlocks_{};
  EEL_F megaError_ = 0.0;
};

}  // namespace avs
