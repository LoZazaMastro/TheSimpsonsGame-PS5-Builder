/* Host-only driver for the actual production recorder and allocator fixtures. */
#include <rex/platform/trace_prospero.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern int heap_fixture_main(int argc, char **argv);
extern void _start(void);

__attribute__((noinline)) void decoder_candidate_marker(void) {
  __asm__ volatile("" ::: "memory");
}

int main(int argc, char **argv) {
  uintptr_t linked;
  __asm__("movabs $__decoder_linked_entry, %0" : "=r"(linked));
  prospero_trace_set_image_anchors((uintptr_t)&_start, linked);
  prospero_trace_set_output_fd(STDOUT_FILENO);
  prospero_trace_checkpoint(100, 0, 0, 0);
  char line[512];
  int length = snprintf(line, sizeof(line),
      "[PPSA74207] METRICS stream=guest_swap pid=%u monotonic_ns=1 fps_avg=60.000 intervals=300\n",
      (unsigned)getpid());
  prospero_trace_write_line(line, (size_t)length);
  length = snprintf(line, sizeof(line),
      "[PPSA74207] HEARTBEAT pid=%u monotonic_ns=1 ui_seen=1 guest_seen=1 present_seen=1\n",
      (unsigned)getpid());
  prospero_trace_write_line(line, (size_t)length);
  if (argc > 1 && strcmp(argv[1], "decoder-stack") == 0) {
    uintptr_t words[4] = {(uintptr_t)&decoder_candidate_marker, 1, 0xdeadbeef, (uintptr_t)&_start};
    prospero_trace_register_stack((uintptr_t)words, sizeof(words));
    prospero_trace_event(PROSPERO_TRACE_PHASE, (uintptr_t)&decoder_candidate_marker, 310, 1, 0, 0);
    prospero_trace_dump_fd(STDOUT_FILENO);
    prospero_trace_dump_stack_fd(STDOUT_FILENO, (uintptr_t)words);
    return 0;
  }
  return heap_fixture_main(argc, argv);
}
