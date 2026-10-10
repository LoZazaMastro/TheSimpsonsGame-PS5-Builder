// Windows compatibility shim for the generated translation units that
// reference POSIX mprotect(). The hand-patch call sites (HandPatchEnsureWritable*)
// are neutered and must do nothing -- doing a real VirtualProtect() here would
// strip the GPU write-watch protection and break texture upload tracking. So
// mprotect() is intentionally a no-op on Windows, matching the neutered intent.
#pragma once
#include <stddef.h>

#define PROT_NONE  0x0
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4

static inline int mprotect(void *addr, size_t len, int prot) {
    (void)addr; (void)len; (void)prot;
    return 0;
}
