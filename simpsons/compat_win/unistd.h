// Minimal POSIX <unistd.h> shim for Windows. Only usleep() is referenced by the
// generated code. Sleep() is declared directly (matching <windows.h>'s
// prototype) rather than including <windows.h>, to avoid macro pollution
// (min/max, near/far, etc.) inside the recompiled translation units.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);
#ifdef __cplusplus
}
#endif

static inline int usleep(unsigned int usec) {
    Sleep((usec + 999u) / 1000u);  // round up to the nearest millisecond
    return 0;
}
