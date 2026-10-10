// Complete native audio driver and real diagnostics; native device/guest-memory
// translation/semaphore boundaries are controlled fixtures, never PS5 execution.
#include <rex/audio/prospero/prospero_audio_driver.h>
#include <rex/audio/prospero/audio_out.h>
#include <rex/platform/audio_metrics_prospero.h>
#include <rex/platform/trace_prospero.h>
#include <rex/audio/flags.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <thread>
#include <string_view>
#include <unistd.h>

namespace fixture {
std::atomic<bool> audio_mute{false};
std::string audio_dump_file;
std::atomic<uint64_t> translated{0};
std::atomic<bool> credit_fail{false};
}
namespace rex::audio {
AudioDriver::AudioDriver(memory::Memory* memory) : memory_(memory) {}
AudioDriver::~AudioDriver() = default;
}

static std::atomic<int> initial_result{0}, open_result{7}, init_result{0};
static std::atomic<unsigned> native_calls{0}, close_calls{0}, active_calls{0}, close_while_active{0};
static std::atomic<unsigned> fail_at{0}, periodic_writes{0};
static std::atomic<bool> paused{false}, record_periodic{false};
static std::atomic<bool> worker_failure_recorded{false};
static std::atomic<uintptr_t> audio_thread{0};
static uintptr_t main_thread = 0;

extern "C" ssize_t __real_write(int, const void*, size_t);
extern "C" ssize_t __wrap_write(int fd, const void* data, size_t length) {
  if (uintptr_t(pthread_self()) != main_thread &&
      std::string_view(static_cast<const char*>(data), length).find("phase=535 ") != std::string_view::npos)
    worker_failure_recorded = true;
  if (record_periodic.load() && uintptr_t(pthread_self()) == audio_thread.load()) {
    periodic_writes.fetch_add(1);
  }
  return __real_write(fd, data, length);
}
extern "C" bool rex_audio_set_thread_priority() { return true; }
extern "C" int32_t sceAudioOutInit() { native_calls = 0; return init_result; }
extern "C" int32_t sceAudioOutOpen(int32_t user, int32_t type, int32_t index,
                                  uint32_t samples, uint32_t rate, uint32_t format) {
  if (user != 255 || type != 0 || index != 0 || samples != 256 || rate != 48000 || format != 1)
    std::abort();
  return open_result;
}
extern "C" int32_t sceAudioOutOutput(int32_t handle, const void* buffer) {
  if (handle != 7 || !buffer) std::abort();
  const unsigned count = native_calls.fetch_add(1) + 1;
  if (uintptr_t(pthread_self()) == main_thread) return initial_result;
  audio_thread = uintptr_t(pthread_self());
  active_calls.fetch_add(1);
  while (paused.load()) std::this_thread::sleep_for(std::chrono::microseconds(100));
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  active_calls.fetch_sub(1);
  return fail_at.load() && count >= fail_at.load() ? int32_t(0x80260006u) : 0;
}
extern "C" int32_t sceAudioOutClose(int32_t handle) {
  if (handle != 7) std::abort();
  close_calls.fetch_add(1);
  if (active_calls.load()) close_while_active.fetch_add(1);
  return 0;
}

static void Check(bool value, const char* text) {
  if (!value) { std::fprintf(stderr, "FAIL %s\n", text); std::exit(1); }
  std::printf("PASS %s\n", text);
}
template <typename Predicate> static void Await(Predicate predicate, const char* text) {
  const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (!predicate() && std::chrono::steady_clock::now() < until)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  Check(predicate(), text);
}

int main(int argc, char** argv) {
  if (argc != 3) return 2;
  main_thread = uintptr_t(pthread_self());
  const std::string mode(argv[1]);
  const int log = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0600);
  Check(log >= 0, "audio trace fixture opens");
  prospero_trace_set_output_fd(log);
  rex::memory::Memory memory;
  rex::thread::Semaphore credits;
  if (mode == "init_fail") init_result = int32_t(0x80260001u);
  if (mode == "open_fail") open_result = int32_t(0x80260002u);
  if (mode == "verify_fail") initial_result = int32_t(0x80260003u);
  if (mode == "queue_full") paused = true;
  if (mode == "reuse") {
    std::atomic<bool> stopping{false};
    std::thread reporter([&] {
      while (!stopping.load()) {
        prospero_audio_report_diagnostics();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });
    for (unsigned i = 0; i < 80; ++i) {
      rex::audio::prospero::ProsperoAudioDriver driver(&memory, &credits);
      Check(driver.Initialize(), "reused driver instance initializes");
      driver.SubmitFrame(0x1000);
      driver.Shutdown();
    }
    stopping = true; reporter.join();
    Check(close_calls == 80 && close_while_active == 0,
          "80 driver retire/reuse cycles and concurrent static reports never close active output");
  } else {
    rex::audio::prospero::ProsperoAudioDriver driver(&memory, &credits);
    const bool initialized = driver.Initialize();
    if (mode == "init_fail" || mode == "open_fail" || mode == "verify_fail") {
      Check(!initialized, "native initialization failure is propagated");
      Check(close_calls == (mode == "verify_fail" ? 1u : 0u),
            "only an actually opened native port is closed after failed verification");
    } else {
      Check(initialized, "complete native audio driver starts");
      if (mode == "queue_full") {
        paused = true;
        Await([] { return active_calls.load() != 0; }, "native output fixture is blocked in one call");
        for (unsigned i = 0; i < 65; ++i) driver.SubmitFrame(0x1000);
        paused = false;
      } else if (mode == "output_fail") {
        fail_at = 3;
        Await([] { return worker_failure_recorded.load(); }, "native output failure stops the worker");
        const auto before = fixture::translated.load();
        driver.SubmitFrame(0x1000);
        Check(fixture::translated == before, "rejected submit after failure never touches guest samples");
      } else if (mode == "credit_fail") {
        fixture::credit_fail = true;
        driver.SubmitFrame(0x1000);
        Await([&] { return credits.attempts.load() != 0; }, "real driver reaches the refused semaphore credit");
      } else if (mode == "healthy") {
        driver.SubmitFrame(0x1000);
        Await([&] { return credits.released.load() == 1 && native_calls.load() > 5; },
              "one actual guest frame returns exactly one semaphore credit");
        record_periodic = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5200));
        record_periodic = false;
        Check(periodic_writes == 0, "paced AudioOut thread performs zero periodic diagnostic writes over 5.2 seconds");
        prospero_audio_report_diagnostics();
        Check(credits.released == 1, "empty output queue never fabricates extra guest credits");
      } else return 2;
      driver.Shutdown();
      Check(close_calls == 1 && close_while_active == 0,
            "shutdown joins the output worker before closing its native port");
    }
  }
  prospero_audio_report_diagnostics();
  prospero_trace_dump_fd(log); prospero_trace_set_output_fd(-1); close(log);
  return 0;
}
