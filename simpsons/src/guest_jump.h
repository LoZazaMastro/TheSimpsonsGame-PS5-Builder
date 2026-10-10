#pragma once

#include <csetjmp>
#include <cstdint>
#include <cstdlib>
#include <unordered_map>

#if defined(__PROSPERO__)
// Use the matched SDK's libc/_setjmp.S pair. Its saved state occupies 72
// bytes, including x87 and MXCSR controls, inside the SDK's jmp_buf. Sony's
// separate setjmp/longjmp exports have no verified buffer contract here.
// The save must remain a returns_twice call in the guest's own frame: a
// helper that returned after _setjmp would leave an invalid resume point.
extern "C" int _setjmp(jmp_buf) __attribute__((returns_twice));
extern "C" void ___longjmp(jmp_buf, int) __attribute__((noreturn));
static_assert(sizeof(jmp_buf) >= 72, "SDK _setjmp state must fit jmp_buf");
#endif

// Windows setjmp saves XMM registers with aligned stores. Some toolchains
// give jmp_buf only 8-byte type alignment, so a map value needs an explicit
// 16-byte boundary regardless of the guest key that precedes it.
struct alignas(16) PPCJumpBuffer {
  jmp_buf value;
};

inline std::unordered_map<uint32_t, PPCJumpBuffer>& get_jmp_buf_map() {
  static thread_local std::unordered_map<uint32_t, PPCJumpBuffer> map;
  return map;
}

#if defined(__PROSPERO__)
#define ppc_setjmp(guest_buf_addr) (_setjmp(::get_jmp_buf_map()[(guest_buf_addr)].value))
#else
#define ppc_setjmp(guest_buf_addr) (setjmp(::get_jmp_buf_map()[(guest_buf_addr)].value))
#endif

[[noreturn]] inline void ppc_longjmp(uint32_t guest_buf_addr, int val) {
  auto& map = get_jmp_buf_map();
  auto it = map.find(guest_buf_addr);
  if (it != map.end()) {
#if defined(__PROSPERO__)
    ___longjmp(it->second.value, val);
#else
    longjmp(it->second.value, val);
#endif
  }
  std::abort();
}
