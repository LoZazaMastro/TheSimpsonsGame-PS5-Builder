// Run an exact generated guest setjmp caller, including its register snapshot.
// The runner extracts sub_82844398 unchanged and supplies the matched SDK jump
// object under isolated symbol names. This fixture supplies guest memory and
// an indirect callee; it does not execute the game or the PS5 loader.
#include "simpsons_init.h"
#include <cstdio>
#include <cstring>
#include <thread>
#include <array>

namespace {
constexpr uint32_t kStack = 0x8000;
constexpr uint32_t kObject = 0x1000;
constexpr uint32_t kTarget = 0x100;  // Exercise the real slow dispatch path.
constexpr uint32_t kSavedBuffer = kStack - 448 + 96;
thread_local unsigned calls = 0;
thread_local bool jump_requested = true;

void Check(bool okay, const char* text) {
  if (!okay) { std::fprintf(stderr, "FAIL %s\n", text); std::abort(); }
}

[[gnu::noinline]] void Callee(PPCContext& ctx, uint8_t* base) {
  ++calls;
  Check(ctx.r3.u32 == kObject && ctx.r4.u32 == 0x12345678,
        "generated function passed unexpected callback arguments");
  if (!jump_requested) return;
  // Clobber the guest's register file just as deeper guest calls may do.
  // The host caller and its snapshot must still be live at the second return.
  std::memset(&ctx, 0xa5, sizeof(ctx));
  ppc_longjmp(kSavedBuffer, 29);
}

void Exercise(unsigned iterations) {
  alignas(64) std::array<uint8_t, 0x10000> memory{};
  for (unsigned i = 0; i < iterations; ++i) {
    auto* base = memory.data();
    PPCContext ctx{};
    ctx.r1.u64 = kStack;
    ctx.r3.u64 = kObject;
    ctx.r4.u64 = kTarget;
    ctx.r5.u64 = 0x12345678;
    ctx.r14.u64 = 0x1122334455667788ull;
    ctx.r20.u64 = 0x33445566778899aaull;
    ctx.r30.u64 = 0x8877665544332211ull;
    ctx.r31.u64 = 0x1234432112344321ull;
    ctx.lr = 0x99887766;
    ctx.f31.u64 = 0x40fabc0000000000ull;
    std::memset(&ctx.v31, 0x5a, sizeof(ctx.v31));
    REX_STORE_U32(kObject + 80, 0x76543210);
    jump_requested = (i & 1) != 0;
    __imp__sub_82844398(ctx, base);
    Check(ctx.r1.u64 == kStack && ctx.r31.u64 == 0x1234432112344321ull &&
          ctx.lr == 0x99887766, "generated epilogue restored wrong guest stack/LR/R31");
    Check(ctx.r14.u64 == 0x1122334455667788ull && ctx.r20.u64 == 0x33445566778899aaull &&
          ctx.r30.u64 == 0x8877665544332211ull && ctx.f31.u64 == 0x40fabc0000000000ull,
          "guest integer or floating-point snapshot changed");
    const auto* vector = reinterpret_cast<const uint8_t*>(&ctx.v31);
    for (size_t byte = 0; byte < sizeof(ctx.v31); ++byte)
      Check(vector[byte] == 0x5a, "guest vector snapshot changed");
    Check(ctx.r3.u32 == 0 && REX_LOAD_U32(kObject + 80) == 0x76543210,
          "exception-chain memory or generated return value changed");
  }
  Check(calls == iterations, "callback was lost or unexpectedly re-entered");
}
}  // namespace

namespace rex::runtime {
PPCFunc* ResolveIndirectFunction(uint32_t address) {
  Check(address == kTarget, "wrong slow-dispatch address");
  return Callee;
}
}  // namespace rex::runtime

int main() {
  constexpr unsigned iterations = 1000;
  std::array<std::thread, 4> threads;
  for (auto& thread : threads) thread = std::thread(Exercise, iterations);
  for (auto& thread : threads) thread.join();
  std::puts("PASS exact generated sub_82844398: 4000 normal/jump calls on four threads; guest registers, stack, LR and exception chain restored");
}
