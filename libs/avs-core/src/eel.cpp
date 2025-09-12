#include "avs/eel.hpp"

#include <algorithm>

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

}  // namespace avs
