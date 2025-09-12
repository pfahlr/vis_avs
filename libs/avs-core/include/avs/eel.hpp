#pragma once

/** \file
 *  \brief Simple wrapper around the NS-EEL virtual machine.
 */

#include <random>
#include <string>

#include "ns-eel-addfuncs.h"
#include "ns-eel.h"

namespace avs {

/** \brief Lightweight RAII wrapper for an NS-EEL VM.
 */
class EelVm {
 public:
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

 private:
  static EEL_F NSEEL_CGEN_CALL funcRand(void* opaque);
  static EEL_F NSEEL_CGEN_CALL funcClamp(void* opaque, EEL_F* x, EEL_F* lo, EEL_F* hi);
  static EEL_F NSEEL_CGEN_CALL funcSmooth(void* opaque, EEL_F* prev, EEL_F* x, EEL_F* a);

  NSEEL_VMCTX ctx_{};
  std::mt19937 rng_{};
};

}  // namespace avs
