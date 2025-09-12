#include "avs/cpu_features.hpp"

#include <atomic>

#if defined(__x86_64__)
#include <cpuid.h>
#endif

namespace avs {

namespace {
bool detectSse2() {
#if defined(__x86_64__)
  unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
  if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) != 0) {
    return (edx & bit_SSE2) != 0;
  }
#endif
  return false;
}
}  // namespace

bool hasSse2() {
  static const bool HAS = detectSse2();
  return HAS;
}

}  // namespace avs
