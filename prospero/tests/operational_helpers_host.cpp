// Real fixed-buffer reporter/rate limiter. Only syscall clock values are fixtures.
#include <rex/platform/trace_game_prospero.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <thread>
#include <unistd.h>

static std::atomic<int64_t> fake_seconds{1000};
static std::atomic<bool> clock_fails{false};
extern "C" int __wrap_clock_gettime(clockid_t, timespec* value) {
  if (clock_fails.load()) { errno = EIO; return -1; }
  value->tv_sec = fake_seconds.load(); value->tv_nsec = 123;
  errno = EACCES;  // The production reporter must restore its caller's errno.
  return 0;
}
static void Check(bool value, const char* message) {
  if (!value) { std::fprintf(stderr, "FAIL %s\n", message); std::exit(1); }
  std::printf("PASS %s\n", message);
}

int main(int argc, char** argv) {
  using namespace rex::prospero::diagnostics;
  if (argc != 2) return 2;
  const int log = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
  Check(log >= 0, "diagnostic log fixture opens");
  prospero_trace_set_output_fd(log);
  errno = ERANGE;
  {
    Line line("path_fixture"); line.Number("requested", UINT64_MAX);
    line.Path("path_pct", "game:\\space %\npretend=field"); line.Write();
  }
  Check(errno == ERANGE, "fixed-buffer path reporting preserves errno after clock/write calls");
  const std::string long_path(10000, '%');
  { Line line("long_path"); line.Path("path_pct", long_path); line.Write(); }
  Check(PathHash("hello") == 0xa430d84680aabd0bull, "FNV-1a correlation hash uses exact byte content");
  RateLimit limit(7);
  std::atomic<unsigned> permitted{0};
  std::array<std::thread, 8> threads;
  for (auto& thread : threads) thread = std::thread([&] {
    for (unsigned i = 0; i != 1000; ++i) {
      if (limit.Allow(PROSPERO_GAME_FILE_OPEN)) permitted.fetch_add(1);
    }
  });
  for (auto& thread : threads) thread.join();
  Check(permitted == 7 && limit.total_suppressed() == 7993,
        "concurrent fixed-second budget permits seven of 8000 calls and counts every suppressed call");
  fake_seconds = 1001;
  Check(limit.Allow(PROSPERO_GAME_FILE_OPEN), "new second permits a report and flushes suppression count");
  fake_seconds = 1000;
  unsigned delayed_permitted = 0;
  for (unsigned i = 0; i < 20; ++i) delayed_permitted += limit.Allow(PROSPERO_GAME_FILE_OPEN);
  Check(delayed_permitted == 6, "late callers cannot reset the window backwards or exceed its budget");
  fake_seconds = 1002; clock_fails = true;
  errno = EDOM;
  Check(!limit.Allow(PROSPERO_GAME_FILE_OPEN) && errno == EDOM,
        "clock failure suppresses the report without leaking errno or flooding output");
  clock_fails = false;
  prospero_trace_dump_fd(log); prospero_trace_set_output_fd(-1); close(log);
  return 0;
}
