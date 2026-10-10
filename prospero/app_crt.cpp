/*
 * Native application startup, adapted from ps5-native-app-boilerplate.
 * Original Copyright (C) 2026 BlackBearReloaded.
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Keep Sonic's floating-point / libc / constructor startup order. The added
 * diagnostics use only native libkernel calls, so the first marker precedes
 * libc initialization, C++ constructors, ReXGlue, and Vulkan.
 */
#include <cstddef>
#include <cstdint>
#include <cerrno>
#include <fcntl.h>
#include <sys/types.h>
#include "../tools/rexglue-sdk/include/rex/platform/trace_phases_prospero.h"
#include "../tools/rexglue-sdk/include/rex/platform/fault_prospero.h"
#include "../tools/rexglue-sdk/include/rex/platform/metrics_prospero.h"

using Destructor = void (*)();
using Initializer = void (*)();

extern "C" {
// Public kernel file API (the PS4-compatible ABI exported by PS5 libkernel).
int sceKernelOpen(const char* path, int flags, mode_t mode);
int sceKernelMkdir(const char* path, mode_t mode);
ssize_t sceKernelWrite(int descriptor, const void* data, std::size_t bytes);
int sceKernelFsync(int descriptor);
int sceKernelClose(int descriptor);
// Declaration matches the pinned SDK's ps5platform/klog.h.
int sceKernelDebugOutText(int channel, const char* text);

void _init_env(void* process_parameters);
void ps5_fp_ieee(void);
int atexit(Destructor callback);
[[noreturn]] void exit(int status);
int application_main(int argc, char** argv, char** envp) __asm__("main");

extern Initializer __preinit_array_start[] __attribute__((weak));
extern Initializer __preinit_array_end[] __attribute__((weak));
extern Initializer __init_array_start[] __attribute__((weak));
extern Initializer __init_array_end[] __attribute__((weak));
extern Initializer __fini_array_start[] __attribute__((weak));
extern Initializer __fini_array_end[] __attribute__((weak));
}

namespace {
constexpr char kAppRoot[] = "/app0";
constexpr char kTitleStateRoot[] = "/data/homebrew/PPSA74207";
constexpr char kAppLogDirectory[] = "/app0/logs";
constexpr char kAppLogPath[] = "/app0/logs/startup.log";
constexpr char kTitleLogDirectory[] = "/data/homebrew/PPSA74207/logs";
constexpr char kTitleLogPath[] = "/data/homebrew/PPSA74207/logs/startup.log";
// Native errors carry the FreeBSD errno in 0x8002xxxx (matched SDK libc.c).
static_assert(EROFS == 30 && EEXIST == 17 && ENOENT == 2, "PS5 native errno values changed");
constexpr std::uint32_t kKernelReadOnly = 0x80020000u | EROFS;
constexpr std::uint32_t kKernelExists = 0x80020000u | EEXIST;
constexpr std::uint32_t kKernelNotFound = 0x80020000u | ENOENT;
constexpr char kReadOnlySelection[] =
    "[PPSA74207] storage: /app0/logs/startup.log open returned EROFS "
    "(0x8002001e); state root = /data/homebrew/PPSA74207\n";
constexpr char kMissingLogSelection[] =
    "[PPSA74207] storage: /app0/logs mkdir returned EROFS (0x8002001e) "
    "and log open returned ENOENT; state root = /data/homebrew/PPSA74207\n";

enum class StateLocation { Unselected, App, TitleFolder };
// The first startup marker fixes this before libc/constructors/threads run.
// Later calls only read it, including after a failed first log open.
StateLocation state_location = StateLocation::Unselected;

const char* log_directory() {
  return state_location == StateLocation::TitleFolder ? kTitleLogDirectory : kAppLogDirectory;
}

const char* log_path() {
  return state_location == StateLocation::TitleFolder ? kTitleLogPath : kAppLogPath;
}

// Compile this translation unit with -fno-builtin: none of these byte loops
// may become strlen/memcpy calls before _init_env has initialized libc.
std::size_t append(char* out, std::size_t at, std::size_t end, const char* text) {
  if (text) {
    while (at < end && *text) {
      out[at++] = *text++;
    }
  }
  return at;
}

std::size_t append_hex(char* out, std::size_t at, std::uint64_t value) {
  constexpr char digits[] = "0123456789abcdef";
  out[at++] = '0';
  out[at++] = 'x';
  for (int shift = 60; shift >= 0; shift -= 4) {
    out[at++] = digits[(value >> shift) & 15];
  }
  return at;
}

void emit_kernel(const char* text, std::size_t size) {
  (void)sceKernelDebugOutText(0, text);
  // Some launchers attach fd 2; kernel debug still receives the record if not.
  (void)sceKernelWrite(2, text, size);
}

void file_error(const char* operation, std::int64_t result) {
  char text[192];
  auto at = append(text, 0, 150, "[PPSA74207] startup.log ");
  at = append(text, at, 150, operation);
  at = append(text, at, 150, " failed, native result ");
  at = append_hex(text, at, static_cast<std::uint64_t>(result));
  text[at++] = '\n';
  text[at] = '\0';
  emit_kernel(text, at);
}

void create_title_log_parents() {
  // Only these exact parents are considered. Existing directories keep their
  // ownership/mode; no other title data are moved or created here.
  const char* const parents[] = {"/data/homebrew", kTitleStateRoot, kTitleLogDirectory};
  for (const char* path : parents) {
    const int result = sceKernelMkdir(path, 0777);
    if (result < 0 && static_cast<std::uint32_t>(result) != kKernelExists) {
      char operation[128];
      auto at = append(operation, 0, sizeof(operation) - 1, "mkdir ");
      at = append(operation, at, sizeof(operation) - 1, path);
      operation[at] = '\0';
      file_error(operation, result);
    }
  }
}

void write_record(int descriptor, const char* text, std::size_t size) {
  std::size_t written = 0;
  while (written < size) {
    const auto result = sceKernelWrite(descriptor, text + written, size - written);
    if (result <= 0 || static_cast<std::size_t>(result) > size - written) {
      file_error("write", result);
      break;
    }
    written += static_cast<std::size_t>(result);
  }
}
}  // namespace

extern "C" const char* prospero_state_root() {
  return state_location == StateLocation::TitleFolder ? kTitleStateRoot : kAppRoot;
}

extern "C" void prospero_startup_log(const char* message) {
  char text[1024];
  auto size = append(text, 0, sizeof(text) - 2, "[PPSA74207] ");
  size = append(text, size, sizeof(text) - 2, message);
  if (size == 0 || text[size - 1] != '\n') {
    text[size++] = '\n';
  }
  text[size] = '\0';
  emit_kernel(text, size);

  const bool first_open = state_location == StateLocation::Unselected;
  if (first_open) state_location = StateLocation::App;
  // Packaging precreates logs. An image builder may omit an empty directory:
  // then mkdir gives EROFS but open gives ENOENT. These two observed errors
  // also prove the first log attempt cannot write the image. Permission errors
  // or later failures never silently switch directories.
  const int mkdir_result = sceKernelMkdir(log_directory(), 0777);
  int descriptor = sceKernelOpen(log_path(), O_WRONLY | O_CREAT | O_APPEND, 0666);
  const bool read_only_open = static_cast<std::uint32_t>(descriptor) == kKernelReadOnly;
  const bool missing_read_only_directory =
      static_cast<std::uint32_t>(mkdir_result) == kKernelReadOnly &&
      static_cast<std::uint32_t>(descriptor) == kKernelNotFound;
  const char* selection_record = nullptr;
  std::size_t selection_size = 0;
  if (first_open && (read_only_open || missing_read_only_directory)) {
    state_location = StateLocation::TitleFolder;
    selection_record = read_only_open ? kReadOnlySelection : kMissingLogSelection;
    selection_size = read_only_open ? sizeof(kReadOnlySelection) - 1 : sizeof(kMissingLogSelection) - 1;
    emit_kernel(selection_record, selection_size);
    create_title_log_parents();
    descriptor = sceKernelOpen(log_path(), O_WRONLY | O_CREAT | O_APPEND, 0666);
  }
  if (descriptor < 0) {
    file_error("open", descriptor);
    return;
  }
  if (selection_record) {
    write_record(descriptor, selection_record, selection_size);
  }
  write_record(descriptor, text, size);
  const int sync_result = sceKernelFsync(descriptor);
  if (sync_result < 0) {
    file_error("fsync", sync_result);
  }
  const int close_result = sceKernelClose(descriptor);
  if (close_result < 0) {
    file_error("close", close_result);
  }
}

namespace {
void log_number(const char* prefix, std::uint64_t value) {
  char text[160];
  auto at = append(text, 0, 130, prefix);
  at = append_hex(text, at, value);
  text[at] = '\0';
  prospero_startup_log(text);
}

void run_forward(Initializer* first, Initializer* last) noexcept {
  if (first == nullptr || last == nullptr) {
    return;
  }
  while (first != last) {
    // If a global constructor faults, the last durable line identifies it
    // against the symbol table of the exact linked ELF.
    log_number("CRT: calling initializer at ", reinterpret_cast<std::uintptr_t>(*first));
    (*first++)();
  }
}

void run_reverse(Initializer* first, Initializer* last) noexcept {
  if (first == nullptr || last == nullptr) {
    return;
  }
  while (last != first) {
    (*--last)();
  }
}
}  // namespace

extern "C" __attribute__((weak)) void catchReturnFromMain(int status) {
  (void)status;
}

extern "C" void _init() {
  prospero_startup_log("CRT: preinitializers begin");
  run_forward(__preinit_array_start, __preinit_array_end);
  prospero_startup_log("CRT: global constructors begin");
  run_forward(__init_array_start, __init_array_end);
  prospero_startup_log("CRT: global constructors completed");
}

extern "C" void _fini() {
  prospero_metrics_stop();
  prospero_startup_log("CRT: finalizers begin");
  run_reverse(__fini_array_start, __fini_array_end);
}

extern "C" [[noreturn]] __attribute__((visibility("default"))) void
_start(void* process_parameters, Destructor loader_teardown) {
  prospero_startup_log("CRT: _start entered");
  prospero_startup_log("CRT: build v3.10.5-ui-menu; updated default profile, English quick settings and framegen hints");
  log_number("CRT: _start runtime address ", reinterpret_cast<std::uintptr_t>(&_start));
  // The linker supplies an ABS symbol. An immediate retains the original
  // ELF address across native conversion/loading; &_start supplies the actual
  // runtime address. Do not turn the absolute number into a relocatable pointer.
  std::uintptr_t linked_entry;
  __asm__("movabs $__prospero_linked_entry, %0" : "=r"(linked_entry));
  log_number("CRT: _start linked ELF address ", linked_entry);
  if (process_parameters == nullptr) {
    prospero_startup_log("CRT: loader supplied null process parameters");
    __builtin_trap();
  }
  const int argc = *static_cast<const int*>(process_parameters);
  auto* parameters = static_cast<std::uint8_t*>(process_parameters);
  auto** argv = reinterpret_cast<char**>(parameters + sizeof(std::uint64_t));

  ps5_fp_ieee();
  prospero_startup_log("CRT: before _init_env");
  _init_env(process_parameters);
  prospero_startup_log("CRT: _init_env completed");
  prospero_trace_set_image_anchors(reinterpret_cast<std::uintptr_t>(&_start), linked_entry);
  // Retain the crash destination before any C++ global constructor can use
  // the heap. The root was selected by the very first native startup write.
  char fault_path[160];
  auto fault_path_end = append(fault_path, 0, sizeof(fault_path) - 1, prospero_state_root());
  fault_path_end = append(fault_path, fault_path_end, sizeof(fault_path) - 1, "/logs/fault.log");
  fault_path[fault_path_end] = '\0';
  if (rex::platform::OpenProsperoFaultLog(fault_path)) {
    prospero_startup_log("diagnostics: crash destination retained before constructors");
  } else {
    log_number("diagnostics: early crash destination errno ", static_cast<unsigned>(errno));
  }
  prospero_trace_event(PROSPERO_TRACE_PHASE, reinterpret_cast<std::uintptr_t>(&_start),
                      PROSPERO_PHASE_CRT_LIBC_READY, 0, 0, 0);
  if (loader_teardown != nullptr) {
    (void)atexit(loader_teardown);
  }
  (void)atexit(_fini);
  _init();
  prospero_trace_event(PROSPERO_TRACE_PHASE, reinterpret_cast<std::uintptr_t>(&_start),
                      PROSPERO_PHASE_CRT_CONSTRUCTORS_DONE, 0, 0, 0);
  prospero_startup_log("CRT: entering main");
  prospero_trace_event(PROSPERO_TRACE_PHASE, reinterpret_cast<std::uintptr_t>(&_start),
                      PROSPERO_PHASE_CRT_MAIN_ENTER, 0, 0, 0);
  const int status = application_main(argc, argv, nullptr);
  // C++ destructors registered by _init() run before the earlier atexit(_fini)
  // callback. Join the reporter here, before exit begins that destruction.
  // _fini keeps an idempotent fallback for explicit exit paths that bypass a
  // return from main; that fallback alone is not a before-all-destructors gate.
  prospero_metrics_stop();
  prospero_trace_checkpoint(PROSPERO_PHASE_CRT_MAIN_EXIT, static_cast<unsigned>(status), 0, 0);
  log_number("CRT: main returned ", static_cast<std::uint32_t>(status));
  catchReturnFromMain(status);
  exit(status);
}
