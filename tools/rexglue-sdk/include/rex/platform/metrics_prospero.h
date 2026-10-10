// Native runtime telemetry. No renderer/UI dependency in the recorder.
// BSD 3-Clause License. See the project LICENSE.
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum ProsperoFrameStream { PROSPERO_FRAME_GUEST_SWAP = 0, PROSPERO_FRAME_PRESENT_OK = 1 };
// start/stop are serialized by application startup and _fini. They are not
// a general concurrent lifecycle API. Frame/UI hooks may run concurrently.
// PRESENT_OK measures successful host queue-present calls, not scanout.
void prospero_metrics_start(void);
void prospero_metrics_stop(void);
void prospero_metrics_ui_tick(void);
void prospero_metrics_frame(unsigned stream);
uint64_t prospero_metrics_clock_ns(void);
void prospero_metrics_present_result(int result, uint64_t started_ns);
#ifdef __cplusplus
}
#endif
