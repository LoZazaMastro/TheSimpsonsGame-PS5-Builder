// Flush static AudioOut snapshots from the independent metrics worker.
// BSD 3-Clause License. No driver pointers are dereferenced by this API.
#pragma once
#ifdef __cplusplus
extern "C" void prospero_audio_report_diagnostics(void) noexcept;
#else
void prospero_audio_report_diagnostics(void);
#endif
