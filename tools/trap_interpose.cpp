// LD_PRELOAD interposer for rex::runtime::ResolveIndirectFunction.
// On a resolution MISS (would return InvalidFunctionTrap), log the guest
// address once and return a harmless no-op so boot continues — letting one
// run enumerate the entire cascade of pointer-only functions the analyzer
// missed, instead of crashing on the first.
#include <dlfcn.h>
#include <cstdint>
#include <cstdio>
#include <unordered_set>
#include <mutex>

extern "C" { static void rex_noop(void*, void*) {} }

typedef void (*PPCFuncPtr)(void*, void*);   // ABI of void(PPCContext&, uint8_t*)
typedef PPCFuncPtr (*resolve_fn)(uint32_t);

static resolve_fn g_real = nullptr;
static PPCFuncPtr g_trap = nullptr;
static FILE* g_log = nullptr;
static std::unordered_set<uint32_t> g_seen;
static std::mutex g_mtx;

static void init_real() {
  g_real = (resolve_fn)dlsym(RTLD_NEXT, "_ZN3rex7runtime23ResolveIndirectFunctionEj");
  if (g_real) g_trap = g_real(0x1u);   // bogus addr -> &InvalidFunctionTrap
  g_log = fopen("/home/deck/simpsons-recomp/missing_funcs.log", "w");
}

namespace rex { namespace runtime {
typedef void PPCFunc(void*, void*);
PPCFunc* ResolveIndirectFunction(unsigned int guest_address) {
  static bool once = (init_real(), true);
  (void)once;
  PPCFuncPtr f = g_real ? g_real(guest_address) : nullptr;
  if (!f || f == g_trap) {
    std::lock_guard<std::mutex> lk(g_mtx);
    if (g_seen.insert(guest_address).second && g_log) {
      fprintf(g_log, "0x%08X\n", guest_address);
      fflush(g_log);
    }
    return (PPCFunc*)rex_noop;
  }
  return (PPCFunc*)f;
}
}}  // namespace rex::runtime
