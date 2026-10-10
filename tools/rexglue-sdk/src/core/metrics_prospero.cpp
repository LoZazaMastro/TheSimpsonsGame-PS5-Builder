// Runtime progress, frame cadence and memory telemetry on an independent thread.
// BSD 3-Clause License. See the project LICENSE.
#include <rex/platform/metrics_prospero.h>
#include <rex/platform/frame_stats_prospero.h>
#include <rex/platform/trace_prospero.h>
#include <rex/platform/trace_phases_prospero.h>
#include <rex/platform/audio_metrics_prospero.h>
#include <ps5platform/heap.h>
#include <ps5platform/shm.h>
#include <atomic>
#include <cerrno>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

namespace {
using rex::platform::prospero::FrameWindow;
constexpr uint64_t kReportNs = 5000000000ULL;
struct Stream {
  std::atomic_flag busy = ATOMIC_FLAG_INIT;
  FrameWindow window;
  uint64_t previous = 0, total = 0;
  std::atomic<uint64_t> last{0}, dropped{0}, clock_rejected{0};
};
constinit Stream streams[2]{};
constinit Stream present_wait{};
constinit std::atomic<uint64_t> ui_last{0}, ui_ticks{0}, present_errors{0};
constinit std::atomic<int> last_present_result{0};
constinit std::atomic<unsigned> started{0}, stopping{0};
pthread_t worker{};

struct Line {
  char bytes[1536];
  size_t used = 0;
  void C(char c) { if (used < sizeof(bytes)) bytes[used++] = c; }
  void S(const char* s) { while (*s) C(*s++); }
  void N(uint64_t n) {
    char digits[20]; unsigned length = 0;
    do { digits[length++] = char('0' + n % 10); n /= 10; } while (n);
    while (length) C(digits[--length]);
  }
  void K(const char* key, uint64_t n) { C(' '); S(key); C('='); N(n); }
  void Milli(const char* key, uint64_t n) {
    C(' '); S(key); C('='); N(n / 1000); C('.');
    C(char('0' + n / 100 % 10)); C(char('0' + n / 10 % 10)); C(char('0' + n % 10));
  }
  void Write() { C('\n'); prospero_trace_write_line(bytes, used); }
};

uint64_t AgeMs(uint64_t now, uint64_t last) {
  return last && now >= last ? (now - last) / 1000000 : 0;
}

void ReportStream(Stream& stream, const char* name, uint64_t now, bool duration_only) {
  if (stream.busy.test_and_set(std::memory_order_acquire)) {
    Line line; line.S("[PPSA74207] METRICS skipped_busy stream="); line.S(name);
    line.K("monotonic_ns", now); line.Write(); return;
  }
  FrameWindow window = stream.window;
  const uint64_t total = stream.total;
  stream.window = FrameWindow{};
  stream.busy.clear(std::memory_order_release);
  window.Sort();
  Line line; line.S("[PPSA74207] METRICS stream="); line.S(name);
  line.K("pid", uint64_t(getpid())); line.K("monotonic_ns", now);
  line.K("total", total); line.K("intervals", window.count);
  line.K("sample_span_ns", window.sum_ns);
  const uint64_t last = stream.last.load(std::memory_order_relaxed);
  line.K("seen", last != 0); line.K("since_last_ms", AgeMs(now, last));
  if (window.count) {
    if (!duration_only) {
      line.Milli("fps_avg", window.AverageFpsMilli());
      line.Milli("fps_min", FrameWindow::RateMilli(1, window.max_ns));
      line.Milli("fps_max", FrameWindow::RateMilli(1, window.min_ns));
      line.Milli("fps_1pct_low", window.OnePercentLowMilli());
    }
    line.Milli("frame_ms_avg", window.sum_ns / window.count / 1000);
    line.Milli("frame_ms_min", window.min_ns / 1000);
    line.Milli("frame_ms_max", window.max_ns / 1000);
    line.Milli("frame_ms_p50", window.Percentile(50) / 1000);
    line.Milli("frame_ms_p95", window.Percentile(95) / 1000);
    line.Milli("frame_ms_p99", window.Percentile(99) / 1000);
  } else {
    line.S(" timing=unavailable no_intervals=1");
  }
  line.K("over_33ms", window.over_33ms); line.K("over_50ms", window.over_50ms);
  line.K("over_100ms", window.over_100ms);
  line.K("distribution_samples", window.stored); line.K("distribution_dropped", window.distribution_dropped);
  line.K("interval_rejected_window", window.rejected);
  line.K("clock_rejected_total", stream.clock_rejected.load(std::memory_order_relaxed));
  line.K("contention_dropped_total", stream.dropped.load(std::memory_order_relaxed)); line.Write();
}

void Report(uint64_t now) {
  ReportStream(streams[0], "guest_swap", now, false);
  ReportStream(streams[1], "present_ok", now, false);
  ReportStream(present_wait, "queue_present_call", now, true);
  const uint64_t ui = ui_last.load(std::memory_order_relaxed);
  const uint64_t guest = streams[0].last.load(std::memory_order_relaxed);
  const uint64_t present = streams[1].last.load(std::memory_order_relaxed);
  Line heart; heart.S("[PPSA74207] HEARTBEAT"); heart.K("pid", uint64_t(getpid()));
  heart.K("monotonic_ns", now); heart.K("ui_seen", ui != 0); heart.K("ui_age_ms", AgeMs(now, ui));
  heart.K("ui_ticks", ui_ticks.load(std::memory_order_relaxed));
  heart.K("guest_seen", guest != 0); heart.K("guest_age_ms", AgeMs(now, guest));
  heart.K("present_seen", present != 0); heart.K("present_age_ms", AgeMs(now, present));
  heart.K("present_errors", present_errors.load(std::memory_order_relaxed));
  heart.K("last_present_result_u32", uint32_t(last_present_result.load(std::memory_order_relaxed)));
  heart.Write();
  struct ps5_heap_stats heap{}; ps5_heap_stats(&heap);
  struct ps5_shm_stats shm{}; ps5_shm_live(&shm);
  Line memory; memory.S("[PPSA74207] MEMORY"); memory.K("pid", uint64_t(getpid()));
  memory.K("monotonic_ns", now); memory.K("heap_range_base", heap.range_base);
  memory.K("heap_reserved_bytes", heap.range_bytes); memory.K("heap_mapped_bytes", heap.mapped_bytes);
  memory.K("heap_peak_bytes", heap.peak_bytes); memory.K("heap_segments", heap.segments);
  memory.K("heap_arenas", heap.arenas); memory.K("libc_fallbacks", heap.libc_fallbacks);
  memory.K("shared_objects", shm.objects); memory.K("shared_object_bytes", shm.object_bytes);
  memory.K("shared_views", shm.views); memory.K("shared_view_bytes", shm.view_bytes);
  memory.K("reserved_ranges", shm.ranges); memory.K("range_bytes", shm.range_bytes);
  memory.K("committed_bytes", shm.committed_bytes);
  memory.S(" snapshot=approximate_atomic_fields accounting=separate_pools_and_virtual_aliases_do_not_sum"); memory.Write();
  // Static snapshots only: no call into a live/destroyed driver object.
  // Periodic diagnostic I/O stays off the paced AudioOut output thread.
  prospero_audio_report_diagnostics();
}

void* Worker(void*) {
  uint64_t last = prospero_metrics_clock_ns();
  // Reports remain active when the UI or guest worker stops making progress.
  Report(last);
  while (!stopping.load(std::memory_order_acquire)) {
    const timespec delay{0, 200000000};
    timespec remaining = delay;
    for (unsigned attempt = 0; attempt < 4 && !stopping.load(std::memory_order_relaxed); ++attempt) {
      if (nanosleep(&remaining, &remaining) == 0) break;
      if (errno != EINTR) {
        prospero_trace_checkpoint(PROSPERO_PHASE_METRICS_WAIT_ERROR, unsigned(errno), 0, 0);
        return nullptr;  // A failed wait must never become a busy loop.
      }
    }
    const uint64_t now = prospero_metrics_clock_ns();
    if (now && now >= last && now - last >= kReportNs) { Report(now); last = now; }
  }
  Report(prospero_metrics_clock_ns());
  return nullptr;
}
}  // namespace

extern "C" uint64_t prospero_metrics_clock_ns(void) {
  const int saved = errno; timespec now{};
  const bool okay = clock_gettime(CLOCK_MONOTONIC, &now) == 0 && now.tv_sec >= 0 &&
                    now.tv_nsec >= 0 && now.tv_nsec < 1000000000 &&
                    uint64_t(now.tv_sec) <= (UINT64_MAX - 999999999) / 1000000000;
  errno = saved;
  return okay ? uint64_t(now.tv_sec) * 1000000000ULL + uint64_t(now.tv_nsec) : 0;
}

extern "C" void prospero_metrics_start(void) {
  unsigned expected = 0;
  if (!started.compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) return;
  stopping.store(0, std::memory_order_release);
  const int result = pthread_create(&worker, nullptr, Worker, nullptr);
  if (result != 0) {
    started.store(0, std::memory_order_release);
    prospero_trace_checkpoint(PROSPERO_PHASE_METRICS_START, unsigned(result), 0, 0);
  } else {
    prospero_trace_checkpoint(PROSPERO_PHASE_METRICS_START, 0, kReportNs, FrameWindow::kCapacity);
  }
}

extern "C" void prospero_metrics_stop(void) {
  if (!started.load(std::memory_order_acquire)) return;
  stopping.store(1, std::memory_order_release);
  const int result = pthread_join(worker, nullptr);
  if (result == 0) started.store(0, std::memory_order_release);
}

extern "C" void prospero_metrics_ui_tick(void) {
  const uint64_t now = prospero_metrics_clock_ns();
  if (now) ui_last.store(now, std::memory_order_relaxed);
  ui_ticks.fetch_add(1, std::memory_order_relaxed);
}

extern "C" void prospero_metrics_frame(unsigned which) {
  if (which > 1) return;
  const uint64_t now = prospero_metrics_clock_ns();
  Stream& stream = streams[which];
  if (!now) { stream.clock_rejected.fetch_add(1, std::memory_order_relaxed); return; }
  if (stream.busy.test_and_set(std::memory_order_acquire)) {
    stream.dropped.fetch_add(1, std::memory_order_relaxed); return;
  }
  ++stream.total;
  if (stream.previous && now > stream.previous) stream.window.Add(now - stream.previous);
  else if (stream.previous) stream.clock_rejected.fetch_add(1, std::memory_order_relaxed);
  stream.previous = now; stream.last.store(now, std::memory_order_relaxed);
  stream.busy.clear(std::memory_order_release);
}

extern "C" void prospero_metrics_present_result(int result, uint64_t began) {
  const uint64_t now = prospero_metrics_clock_ns();
  last_present_result.store(result, std::memory_order_relaxed);
  if (result != 0 && result != 1000001003) present_errors.fetch_add(1, std::memory_order_relaxed);
  else prospero_metrics_frame(PROSPERO_FRAME_PRESENT_OK);  // VK_SUCCESS / VK_SUBOPTIMAL_KHR.
  if (!now || !began || now <= began) return;
  if (present_wait.busy.test_and_set(std::memory_order_acquire)) {
    present_wait.dropped.fetch_add(1, std::memory_order_relaxed); return;
  }
  present_wait.window.Add(now - began); ++present_wait.total;
  present_wait.last.store(now, std::memory_order_relaxed);
  present_wait.busy.clear(std::memory_order_release);
}
