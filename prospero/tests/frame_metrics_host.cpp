// Synthetic statistics and the production independent telemetry worker.
#include <rex/platform/frame_stats_prospero.h>
#include <rex/platform/metrics_prospero.h>
#include <ps5platform/heap.h>
#include <ps5platform/shm.h>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <string>
#include <time.h>
#include <unistd.h>

static int output = -1;
static std::atomic<unsigned> line_count{0};
static std::atomic<unsigned> audio_reports{0};
static std::atomic<bool> audio_report_on_main{false};
static pthread_t main_thread{};
// Only the scheduling seam is mocked here. The actual audio snapshot cache
// and driver are exercised by the dedicated operational diagnostics runner.
extern "C" void prospero_audio_report_diagnostics(void) noexcept {
  if (pthread_equal(pthread_self(), main_thread)) audio_report_on_main.store(true);
  audio_reports.fetch_add(1, std::memory_order_relaxed);
}
extern "C" void prospero_trace_write_line(const char* data, size_t length) noexcept {
  if (write(output, data, length) != static_cast<ssize_t>(length)) _exit(81);
  line_count.fetch_add(1, std::memory_order_relaxed);
}
extern "C" void prospero_trace_checkpoint(uintptr_t, uintptr_t, uintptr_t, uintptr_t) noexcept {}
extern "C" void ps5_heap_stats(struct ps5_heap_stats* stats) {
  *stats = {}; stats->mapped_bytes = 123456; stats->peak_bytes = 234567;
  stats->range_bytes = 16ULL << 30; stats->arenas = 8; stats->libc_fallbacks = 3;
}
extern "C" void ps5_shm_live(struct ps5_shm_stats* stats) {
  *stats = {}; stats->object_bytes = 456789; stats->view_bytes = 3 * stats->object_bytes;
}
static void Check(bool okay, const char* message) {
  if (!okay) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static void Sleep(long nanoseconds) {
  timespec delay{nanoseconds / 1000000000, nanoseconds % 1000000000};
  while (nanosleep(&delay, &delay) && errno == EINTR) {}
}
int main() {
  main_thread = pthread_self();
  using rex::platform::prospero::FrameWindow;
  FrameWindow sixty;
  for (unsigned i = 0; i < 300; ++i) sixty.Add(16666667);
  sixty.Sort();
  Check(sixty.AverageFpsMilli() == 60000 && sixty.OnePercentLowMilli() == 60000,
        "synthetic steady 60Hz average and low");
  Check(sixty.Percentile(99) == 16666667 && sixty.min_ns == sixty.max_ns,
        "steady percentiles/extremes");
  FrameWindow stalled;
  for (unsigned i = 0; i < 99; ++i) stalled.Add(16666667);
  stalled.Add(100000000); stalled.Sort();
  Check(stalled.AverageFpsMilli() == 57143 && stalled.OnePercentLowMilli() == 10000,
        "one slow frame changes weighted FPS and one-percent low correctly");
  Check(FrameWindow::RateMilli(1, stalled.max_ns) == 10000 &&
        FrameWindow::RateMilli(1, stalled.min_ns) == 60000 &&
        stalled.Percentile(99) == 16666667 && stalled.Percentile(100) == 100000000,
        "instantaneous extrema and nearest-rank percentiles");
  Check(stalled.over_33ms == 1 && stalled.over_50ms == 1 && stalled.over_100ms == 0,
        "strict long-frame thresholds");
  FrameWindow bounded;
  for (unsigned i = 0; i < 1124; ++i) bounded.Add(16666667);
  Check(bounded.count == 1124 && bounded.stored == 1024 && bounded.distribution_dropped == 100 &&
        bounded.AverageFpsMilli() == 60000, "bounded distribution, full sum/count and explicit loss");
  FrameWindow extreme; extreme.Add(0); extreme.Add(UINT64_MAX); extreme.Add(1);
  Check(extreme.rejected == 2 && extreme.count == 1, "zero/overflow rejected without wrap");
  FrameWindow empty;
  Check(empty.AverageFpsMilli() == 0 && empty.OnePercentLowMilli() == 0 && empty.Percentile(99) == 0,
        "empty statistics defined");
  std::puts("PASS synthetic frame distributions: steady cadence, stutter, extrema, 1% low, percentiles, bounds");

  char path[] = "/tmp/simpsons-metrics-XXXXXX";
  output = mkstemp(path); Check(output >= 0, "output fixture");
  const uint64_t before = prospero_metrics_clock_ns();
  prospero_metrics_start();
  // No UI or renderer calls: the independent worker must still report twice.
  Sleep(5300000000L);
  Check(line_count.load(std::memory_order_relaxed) >= 10,
        "independent progress/memory reports continue without game/UI frames");
  for (unsigned i = 0; i < 15; ++i) {
    prospero_metrics_ui_tick();
    prospero_metrics_frame(PROSPERO_FRAME_GUEST_SWAP);
    const uint64_t start = prospero_metrics_clock_ns();
    Sleep(1000000);
    prospero_metrics_present_result(i == 4 ? -4 : 0, start);
    Sleep(3000000);
  }
  prospero_metrics_stop();
  prospero_metrics_stop();
  Check(audio_reports.load() >= 3 && !audio_report_on_main.load(),
        "audio snapshot flushing scheduled on independent worker including final report");
  Check(prospero_metrics_clock_ns() > before, "monotonic clock");
  Check(lseek(output, 0, SEEK_SET) == 0, "read fixture");
  std::string contents;
  char block[4096];
  for (ssize_t size; (size = read(output, block, sizeof(block))) > 0;) contents.append(block, size);
  Check(contents.find("timing=unavailable no_intervals=1") != std::string::npos,
        "no fabricated zero-frame FPS");
  Check(contents.find("stream=guest_swap") != std::string::npos &&
        contents.find("stream=present_ok") != std::string::npos &&
        contents.find("stream=queue_present_call") != std::string::npos,
        "game requests, successful presentation and host call time distinguished");
  Check(contents.find("present_errors=1") != std::string::npos &&
        contents.find("fps_1pct_low=") != std::string::npos &&
        contents.find("frame_ms_p99=") != std::string::npos,
        "presentation failure and distribution data emitted");
  Check(contents.find("heap_mapped_bytes=123456") != std::string::npos &&
        contents.find("shared_object_bytes=456789") != std::string::npos &&
        contents.find("shared_view_bytes=1370367") != std::string::npos &&
        contents.find("separate_pools_and_virtual_aliases_do_not_sum") != std::string::npos,
        "memory categories retain units without false total usage");
  const char* copy = std::getenv("METRICS_CAPTURE");
  if (copy) {
    const int saved = open(copy, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    Check(saved >= 0 && write(saved, contents.data(), contents.size()) == ssize_t(contents.size()), "capture");
    close(saved);
  }
  close(output); unlink(path);
  std::puts("PASS production worker: independent 5s heartbeat, sample logs, memory, error count, orderly double stop");
  std::puts("All performance numbers above come from synthetic/host tests, not a PS5 game run.");
}
