// Compile app_crt.cpp with _start/_init/_fini renamed, -ffunction-sections,
// -fno-builtin, and link with --gc-sections. Only the real logger is exercised.
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <cstdio>
#include <sys/types.h>

extern "C" void prospero_startup_log(const char* message);
extern "C" const char* prospero_state_root();

namespace {
char file_bytes[8192];
std::size_t file_size;
char kernel_messages[32][1024];
int kernel_count;
int opens, closes, syncs, writes;
int app_opens, title_opens;
constexpr int kReadOnly = static_cast<int>(0x8002001eu);
constexpr int kNotFound = static_cast<int>(0x80020002u);
constexpr int kDenied = static_cast<int>(0x8002000du);
constexpr int kExists = static_cast<int>(0x80020011u);
int app_open_result = 17;
int title_open_result = 17;
int app_mkdir_result = kExists;
int title_mkdir_result = kExists;
int sync_result;
int close_result;
ssize_t write_result = 7;

void reset() {
  file_size = 0;
  kernel_count = opens = closes = syncs = writes = 0;
  app_opens = title_opens = 0;
  app_open_result = title_open_result = 17;
  app_mkdir_result = title_mkdir_result = kExists;
  sync_result = close_result = 0;
  write_result = 7;
}
bool contains(const char* text) {
  for (int i = 0; i < kernel_count; ++i) {
    if (std::strstr(kernel_messages[i], text)) {
      return true;
    }
  }
  return false;
}
}  // namespace

extern "C" int sceKernelOpen(const char* path, int flags, mode_t mode) {
  assert(flags == (O_WRONLY | O_CREAT | O_APPEND));
  assert(mode == 0666);
  ++opens;
  if (std::strcmp(path, "/app0/logs/startup.log") == 0) {
    ++app_opens;
    return app_open_result;
  }
  assert(std::strcmp(path, "/data/homebrew/PPSA74207/logs/startup.log") == 0);
  ++title_opens;
  return title_open_result;
}
extern "C" int sceKernelMkdir(const char* path, mode_t mode) {
  assert(mode == 0777);
  if (std::strcmp(path, "/app0/logs") == 0) return app_mkdir_result;
  assert(std::strcmp(path, "/data/homebrew") == 0 ||
         std::strcmp(path, "/data/homebrew/PPSA74207") == 0 ||
         std::strcmp(path, "/data/homebrew/PPSA74207/logs") == 0);
  return title_mkdir_result;
}
extern "C" ssize_t sceKernelWrite(int descriptor, const void* data, std::size_t bytes) {
  if (descriptor == 2) {
    return -1;  // Missing launcher stderr must not prevent file/kernel logging.
  }
  assert(descriptor == 17);
  ++writes;
  if (write_result <= 0) {
    return write_result;
  }
  const auto count = bytes < static_cast<std::size_t>(write_result)
                         ? bytes : static_cast<std::size_t>(write_result);
  assert(file_size + count < sizeof(file_bytes));
  std::memcpy(file_bytes + file_size, data, count);
  file_size += count;
  file_bytes[file_size] = '\0';
  return static_cast<ssize_t>(count);
}
extern "C" int sceKernelFsync(int descriptor) {
  assert(descriptor == 17);
  ++syncs;
  return sync_result;
}
extern "C" int sceKernelClose(int descriptor) {
  assert(descriptor == 17);
  ++closes;
  return close_result;
}
extern "C" int sceKernelDebugOutText(int channel, const char* message) {
  assert(channel == 0 && kernel_count < 32);
  assert(std::strlen(message) < sizeof(kernel_messages[0]));
  std::strcpy(kernel_messages[kernel_count++], message);
  return 0;
}

int main(int argc, char** argv) {
  assert(argc == 2);
  // Each mode runs in a fresh process, just like a native title launch. The
  // production selector has no reset hook and must never change after entry.
  assert(std::strcmp(prospero_state_root(), "/app0") == 0);
  if (std::strcmp(argv[1], "read-only") == 0 || std::strcmp(argv[1], "missing-logs") == 0) {
    reset();
    const bool missing_logs = std::strcmp(argv[1], "missing-logs") == 0;
    app_mkdir_result = kReadOnly;
    app_open_result = missing_logs ? kNotFound : kReadOnly;
    prospero_startup_log("first marker");
    assert(std::strcmp(prospero_state_root(), "/data/homebrew/PPSA74207") == 0);
    assert(app_opens == 1 && title_opens == 1 && syncs == 1 && closes == 1);
    assert(contains("EROFS (0x8002001e)") && contains("state root = /data/homebrew/PPSA74207"));
    if (missing_logs) assert(contains("mkdir returned EROFS") && contains("open returned ENOENT"));
    assert(std::strstr(file_bytes, "EROFS (0x8002001e)") &&
           std::strstr(file_bytes, "[PPSA74207] first marker\n"));
    app_open_result = 17;  // A later change cannot redirect live state again.
    prospero_startup_log("second marker");
    assert(app_opens == 1 && title_opens == 2 && syncs == 2 && closes == 2);
    std::puts(missing_logs
                  ? "PASS omitted logs directory: native mkdir EROFS plus open ENOENT selects title root"
                  : "PASS read-only: exact native EROFS selects one stable title root and durable cause");
    return 0;
  }
  if (std::strcmp(argv[1], "denied") == 0 || std::strcmp(argv[1], "other-error") == 0) {
    reset();
    app_open_result = std::strcmp(argv[1], "denied") == 0 ? kDenied : -30;
    if (app_open_result == kDenied) app_mkdir_result = kReadOnly;
    prospero_startup_log("failed first open");
    assert(std::strcmp(prospero_state_root(), "/app0") == 0);
    assert(app_opens == 1 && title_opens == 0 && file_size == 0);
    assert(contains("open failed, native result") && !contains("state root ="));
    app_open_result = kReadOnly;
    prospero_startup_log("later error");
    assert(app_opens == 2 && title_opens == 0 &&
           std::strcmp(prospero_state_root(), "/app0") == 0);
    std::puts("PASS non-EROFS first open: no fallback for permissions, POSIX codes or later errors");
    return 0;
  }
  if (std::strcmp(argv[1], "fallback-denied") == 0) {
    reset();
    app_open_result = app_mkdir_result = kReadOnly;
    title_open_result = title_mkdir_result = kDenied;
    prospero_startup_log("before runtime");
    assert(std::strcmp(prospero_state_root(), "/data/homebrew/PPSA74207") == 0);
    assert(app_opens == 1 && title_opens == 1 && writes == 0 && closes == 0 && syncs == 0);
    assert(contains("state root = /data/homebrew/PPSA74207") &&
           contains("mkdir /data/homebrew/PPSA74207 failed") &&
           contains("open failed, native result 0xffffffff8002000d"));
    std::puts("PASS unavailable selected title root: reports native failures without another fallback");
    return 0;
  }
  assert(std::strcmp(argv[1], "writable") == 0);
  reset();
  app_mkdir_result = kReadOnly;  // Selection is based only on the open result.
  prospero_startup_log("first");
  prospero_startup_log("second\n");
  assert(std::strcmp(file_bytes, "[PPSA74207] first\n[PPSA74207] second\n") == 0);
  assert(opens == 2 && syncs == 2 && closes == 2 && writes > 2);
  assert(title_opens == 0 && std::strcmp(prospero_state_root(), "/app0") == 0);

  reset();
  app_open_result = kReadOnly;
  prospero_startup_log("before libc");
  assert(contains("before libc") && contains("open failed, native result"));
  assert(writes == 0 && closes == 0 && syncs == 0);
  assert(title_opens == 0 && std::strcmp(prospero_state_root(), "/app0") == 0);

  const int failures[] = {0, -5};
  for (const int failure : failures) {
    reset();
    write_result = failure;
    prospero_startup_log("failed write");
    assert(contains("write failed, native result"));
    assert(writes == 1 && syncs == 1 && closes == 1);
  }

  reset();
  sync_result = -5;
  close_result = -9;
  prospero_startup_log("flush errors");
  assert(contains("fsync failed") && contains("close failed"));

  reset();
  char long_message[2048];
  std::memset(long_message, 'x', sizeof(long_message) - 1);
  long_message[sizeof(long_message) - 1] = '\0';
  prospero_startup_log(long_message);
  assert(file_size == 1023 && file_bytes[1022] == '\n');
  prospero_startup_log(nullptr);
  assert(closes == 2 && syncs == 2);
  std::puts("startup logger host boundary: PASS (append, partial writes, native failures, no stderr, bounded records)");
}
