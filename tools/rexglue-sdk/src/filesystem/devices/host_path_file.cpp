/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/filesystem/devices/host_path_entry.h>
#include <rex/filesystem/devices/host_path_file.h>
#if REX_PLATFORM_PROSPERO
#include <rex/platform/trace_game_prospero.h>
#endif

namespace rex::filesystem {

namespace {
X_STATUS TraceGuestIoError(const Entry* entry, const char* operation, size_t offset,
                           size_t requested, X_STATUS status) {
#if REX_PLATFORM_PROSPERO
  static rex::prospero::diagnostics::RateLimit errors{16};
  if (errors.Allow(PROSPERO_GAME_GUEST_IO_ERROR)) {
    const auto hash = rex::prospero::diagnostics::PathHash(entry->absolute_path());
    prospero_trace_checkpoint(PROSPERO_GAME_GUEST_IO_ERROR, hash, offset, status);
    rex::prospero::diagnostics::Line line("guest_io_error");
    line.Number("path_hash", hash); line.Number("status", status);
    line.Number("offset", offset); line.Number("requested", requested);
    line.Number("suppressed_total", errors.total_suppressed());
    line.Path("operation", operation); line.Path("path_pct", entry->absolute_path()); line.Write();
  }
#else
  (void)entry; (void)operation; (void)offset; (void)requested;
#endif
  return status;
}
}

HostPathFile::HostPathFile(uint32_t file_access, HostPathEntry* entry,
                           std::unique_ptr<rex::filesystem::FileHandle> file_handle)
    : File(file_access, entry), file_handle_(std::move(file_handle)) {}

HostPathFile::~HostPathFile() = default;

void HostPathFile::Destroy() {
  delete this;
}

X_STATUS HostPathFile::ReadSync(std::span<uint8_t> buffer, size_t byte_offset,
                                size_t* out_bytes_read) {
  if (!(file_access_ & (FileAccess::kGenericRead | FileAccess::kFileReadData))) {
    return TraceGuestIoError(entry_, "read", byte_offset, buffer.size(), X_STATUS_ACCESS_DENIED);
  }

  if (file_handle_->Read(byte_offset, buffer.data(), buffer.size(), out_bytes_read)) {
    return X_STATUS_SUCCESS;
  } else {
    return TraceGuestIoError(entry_, "read", byte_offset, buffer.size(), X_STATUS_END_OF_FILE);
  }
}

X_STATUS HostPathFile::WriteSync(std::span<const uint8_t> buffer, size_t byte_offset,
                                 size_t* out_bytes_written) {
  if (!(file_access_ &
        (FileAccess::kGenericWrite | FileAccess::kFileWriteData | FileAccess::kFileAppendData))) {
    return TraceGuestIoError(entry_, "write", byte_offset, buffer.size(), X_STATUS_ACCESS_DENIED);
  }

  if (file_handle_->Write(byte_offset, buffer.data(), buffer.size(), out_bytes_written)) {
    return X_STATUS_SUCCESS;
  } else {
    return TraceGuestIoError(entry_, "write", byte_offset, buffer.size(), X_STATUS_END_OF_FILE);
  }
}

X_STATUS HostPathFile::SetLength(size_t length) {
  if (!(file_access_ & (FileAccess::kGenericWrite | FileAccess::kFileWriteData))) {
    return TraceGuestIoError(entry_, "truncate", length, 0, X_STATUS_ACCESS_DENIED);
  }

  if (file_handle_->SetLength(length)) {
    return X_STATUS_SUCCESS;
  } else {
    return TraceGuestIoError(entry_, "truncate", length, 0, X_STATUS_END_OF_FILE);
  }
}

}  // namespace rex::filesystem
