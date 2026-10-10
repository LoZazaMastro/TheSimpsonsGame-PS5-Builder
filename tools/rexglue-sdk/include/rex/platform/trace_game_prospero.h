// Native PS5 operational diagnostics. BSD 3-Clause License.
// Phase codes 500..699 are reserved here; 400/401 are terminal diagnostics.
#pragma once
#include "trace_prospero.h"

enum ProsperoGameTracePhase {
  PROSPERO_GAME_INPUT_INIT = 500,       // b=stage, c=native result, d=0
  PROSPERO_GAME_INPUT_USER = 501,       // b=slot, c=user id, d=assigned(1)/removed(0)
  PROSPERO_GAME_INPUT_CONNECTION = 502,// b=PS5 user id, c=handle, d=connected
  PROSPERO_GAME_INPUT_BUTTONS = 503,    // b=slot, c=old buttons, d=new buttons
  PROSPERO_GAME_INPUT_ANALOG = 504,     // b=(coalesced<<32)|slot, c=LX/LY/RX/RY u16, d=LT/RT u8
  PROSPERO_GAME_INPUT_CAPTURE = 505,    // b=slot, c=captured, d=connected
  PROSPERO_GAME_INPUT_RUMBLE = 506,     // b=user id, c=large/small u8, d=native result
  PROSPERO_GAME_INPUT_ERROR = 507,     // b=user id, c=operation hash, d=native result
  PROSPERO_GAME_AUDIO_INIT = 530,       // b=stage(0 begin/1 open/2 ready), c=port, d=result
  PROSPERO_GAME_AUDIO_FIRST_SUBMIT = 531,// b=driver, c=guest samples pointer, d=0
  PROSPERO_GAME_AUDIO_WORKER = 532,     // b=driver, c=start(1)/exit(0), d=port
  PROSPERO_GAME_AUDIO_STATS = 533,      // b=successful outputs, c=empty queue periods, d=silent guest frames
  PROSPERO_GAME_AUDIO_QUEUE = 534,      // b=submitted, c=rejected, d=(peak<<32)|queued
  PROSPERO_GAME_AUDIO_ERROR = 535,      // b=stage, c=result, d=port
  PROSPERO_GAME_AUDIO_SHUTDOWN = 536,   // b=driver, c=submitted, d=rejected
  PROSPERO_GAME_FILE_OPEN = 560,        // b=path hash, c=native flags, d=(errno<<32)|fd
  PROSPERO_GAME_FILE_READ_ERROR = 561,  // b=path hash, c=offset, d=(errno<<32)|requested low32
  PROSPERO_GAME_FILE_WRITE_ERROR = 562, // same tuple as read error
  PROSPERO_GAME_FILE_SHORT = 563,       // b=path hash, c=requested, d=completed
  PROSPERO_GAME_FILE_CLOSE = 564,       // b=path hash, c=read bytes, d=written bytes
  PROSPERO_GAME_FILE_OTHER_ERROR = 565, // b=path hash, c=operation hash, d=errno
  PROSPERO_GAME_VFS_OPEN = 566,         // b=guest path hash, c=desired access, d=X_STATUS
  PROSPERO_GAME_VFS_MOUNT = 567,        // b=path hash, c=target hash (link) / 0 (device), d=read only
  PROSPERO_GAME_GUEST_IO_ERROR = 568,   // b=guest path hash, c=offset, d=X_STATUS
  PROSPERO_GAME_LOG_SUPPRESSED = 569,   // b=stream phase, c=suppressed since previous permitted event, d=0
  PROSPERO_GAME_PACING_MODE = 590,      // b=FPS cap, c=no recent physics step, d=scheduler enabled
  PROSPERO_GAME_DAC_MIXER = 591,        // b=entry(1)/return(0), c/d=0
};

#ifdef __cplusplus
#include <atomic>
#include <cerrno>
#include <chrono>
#include <string_view>
#include <time.h>
#include <unistd.h>

namespace rex::prospero::diagnostics {

// These helpers are for normal operational logging, never signal/allocator paths.
// Hashes correlate numeric ring entries with full paths in the regular log;
// collisions are possible, so they are not a replacement for the path text.
inline uint64_t PathHash(std::string_view text) noexcept {
  uint64_t hash = 14695981039346656037ull;
  for (const unsigned char value : text) {
    hash = (hash ^ value) * 1099511628211ull;
  }
  return hash;
}

struct PreserveErrno {
  int value = errno;
  ~PreserveErrno() { errno = value; }
};

// Fixed-buffer operational line. All text values are percent-encoded so a
// path with spaces cannot create fake key/value fields. Reserving a suffix
// makes truncation explicit. This normal-context writer does not call stdio,
// allocate, or enter the regular logger; the shared writer preserves errno.
class Line {
 public:
  explicit Line(const char* category) noexcept {
    Text("[PPSA74207] OPS category="); Text(category);
    Number("pid", uint64_t(getpid()));
    timespec now{};
    const bool valid = clock_gettime(CLOCK_MONOTONIC, &now) == 0 && now.tv_sec >= 0 &&
        now.tv_nsec >= 0 && now.tv_nsec < 1000000000 &&
        uint64_t(now.tv_sec) <= (UINT64_MAX - 999999999) / 1000000000;
    Number("monotonic_ns", valid ? uint64_t(now.tv_sec) * 1000000000 + now.tv_nsec : 0);
  }
  void Number(const char* key, uint64_t value) noexcept {
    Character(' '); Text(key); Character('=');
    char digits[20]; unsigned length = 0;
    do { digits[length++] = char('0' + value % 10); value /= 10; } while (value);
    while (length) Character(digits[--length]);
  }
  void Path(const char* key, std::string_view value) noexcept {
    Character(' '); Text(key); Character('=');
    constexpr char hex[] = "0123456789ABCDEF";
    for (const unsigned char c : value) {
      if (used_ + 3 > sizeof(bytes_) - 32) { truncated_ = true; break; }
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '/' || c == ':') {
        Character(char(c));
      } else {
        Character('%'); Character(hex[c >> 4]); Character(hex[c & 15]);
      }
    }
  }
  void Write() noexcept {
    // Bypass Character's reserved suffix budget for this fixed, short tail.
    constexpr char tail[] = " truncated=";
    for (char c : std::string_view(tail, sizeof(tail) - 1)) bytes_[used_++] = c;
    bytes_[used_++] = truncated_ ? '1' : '0';
    bytes_[used_++] = '\n';
    prospero_trace_write_line(bytes_, used_);
  }
 private:
  void Character(char value) noexcept {
    if (used_ < sizeof(bytes_) - 32) bytes_[used_++] = value;
    else truncated_ = true;
  }
  void Text(const char* value) noexcept { while (*value) Character(*value++); }
  PreserveErrno preserve_;
  char bytes_[1536];
  size_t used_ = 0;
  bool truncated_ = false;
};

// Fixed per-second budget. No allocation or waiting for another writer. A
// delayed caller cannot reset the window backwards; eight failed CAS attempts
// count as suppression. The next permitted event reports accumulated drops.
class RateLimit {
 public:
  explicit constexpr RateLimit(uint32_t per_second) : limit_(per_second) {}
  bool Allow(uintptr_t stream_phase) noexcept {
    PreserveErrno preserve;
    timespec now{};
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0 || now.tv_sec < 0 ||
        now.tv_nsec < 0 || now.tv_nsec >= 1000000000) {
      suppressed_.fetch_add(1, std::memory_order_relaxed);
      total_suppressed_.fetch_add(1, std::memory_order_relaxed);
      return false;
    }
    const uint32_t second = static_cast<uint32_t>(now.tv_sec);
    uint64_t old = window_.load(std::memory_order_relaxed);
    for (unsigned attempt = 0; attempt != 8; ++attempt) {
      const uint32_t prior = static_cast<uint32_t>(old >> 32);
      const uint32_t elapsed = second - prior;
      const bool newer = elapsed && elapsed < 0x80000000u;
      const uint64_t next = newer ? (uint64_t(second) << 32) | 1 : old + 1;
      if (!newer && static_cast<uint32_t>(old) >= limit_) break;
      if (window_.compare_exchange_weak(old, next, std::memory_order_relaxed)) {
        const uint64_t suppressed = suppressed_.exchange(0, std::memory_order_relaxed);
        if (suppressed) {
          prospero_trace_checkpoint(PROSPERO_GAME_LOG_SUPPRESSED, stream_phase, suppressed, 0);
        }
        return true;
      }
    }
    suppressed_.fetch_add(1, std::memory_order_relaxed);
    total_suppressed_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }
  uint64_t total_suppressed() const noexcept {
    return total_suppressed_.load(std::memory_order_relaxed);
  }
 private:
  const uint32_t limit_;
  std::atomic<uint64_t> window_{0};
  std::atomic<uint64_t> suppressed_{0};
  std::atomic<uint64_t> total_suppressed_{0};
};
static_assert(std::atomic<uint64_t>::is_always_lock_free);

}  // namespace rex::prospero::diagnostics
#endif
