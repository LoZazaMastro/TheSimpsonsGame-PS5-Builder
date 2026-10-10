/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/filesystem/vfs.h>
#include <rex/logging.h>
#include <rex/string.h>

#include <rex/filesystem/devices/host_path_entry.h>
#if REX_PLATFORM_PROSPERO
#include <rex/platform/trace_game_prospero.h>
#endif

REXCVAR_DEFINE_BOOL(allow_game_relative_writes, false, "Filesystem",
                    "Not useful to non-developers. Allows code to write to paths "
                    "relative to game://. Used for "
                    "generating test data to compare with original hardware.");

namespace rex::filesystem {

#if REX_PLATFORM_PROSPERO
namespace {
rex::prospero::diagnostics::RateLimit vfs_open_logs{32}, vfs_error_logs{16}, vfs_mount_logs{16};
}
#endif

VirtualFileSystem::VirtualFileSystem() {}

VirtualFileSystem::~VirtualFileSystem() {
  // Delete all devices.
  // This will explode if anyone is still using data from them.
  devices_.clear();
  symlinks_.clear();
}

bool VirtualFileSystem::RegisterDevice(std::unique_ptr<Device> device) {
  auto global_lock = global_critical_region_.Acquire();
#if REX_PLATFORM_PROSPERO
  if (vfs_mount_logs.Allow(PROSPERO_GAME_VFS_MOUNT)) {
    const auto hash = rex::prospero::diagnostics::PathHash(device->mount_path());
    prospero_trace_checkpoint(PROSPERO_GAME_VFS_MOUNT, hash, 0, device->is_read_only());
    rex::prospero::diagnostics::Line line("vfs_device");
    line.Number("path_hash", hash); line.Number("read_only", device->is_read_only());
    line.Number("suppressed_total", vfs_mount_logs.total_suppressed());
    line.Path("path_pct", device->mount_path()); line.Write();
  }
#endif
  devices_.emplace_back(std::move(device));
  return true;
}

bool VirtualFileSystem::UnregisterDevice(const std::string_view path) {
  auto global_lock = global_critical_region_.Acquire();
  for (auto it = devices_.begin(); it != devices_.end(); ++it) {
    if ((*it)->mount_path() == path) {
      REXFS_DEBUG("Unregistered device: {}", (*it)->mount_path());
      devices_.erase(it);
      return true;
    }
  }
  return false;
}

bool VirtualFileSystem::RegisterSymbolicLink(const std::string_view path,
                                             const std::string_view target) {
  auto global_lock = global_critical_region_.Acquire();
  symlinks_.insert({std::string(path), std::string(target)});
  REXFS_DEBUG("Registered symbolic link: {} => {}", path, target);
#if REX_PLATFORM_PROSPERO
  if (vfs_mount_logs.Allow(PROSPERO_GAME_VFS_MOUNT)) {
    prospero_trace_checkpoint(PROSPERO_GAME_VFS_MOUNT,
        rex::prospero::diagnostics::PathHash(path), rex::prospero::diagnostics::PathHash(target), 0);
    rex::prospero::diagnostics::Line line("vfs_link");
    line.Number("suppressed_total", vfs_mount_logs.total_suppressed());
    line.Path("path_pct", path); line.Path("target_pct", target); line.Write();
  }
#endif

  return true;
}

bool VirtualFileSystem::UnregisterSymbolicLink(const std::string_view path) {
  auto global_lock = global_critical_region_.Acquire();
  auto it = std::find_if(symlinks_.cbegin(), symlinks_.cend(), [&](const auto& s) {
    return rex::string::utf8_equal_case(path, s.first);
  });
  if (it == symlinks_.end()) {
    return false;
  }
  REXFS_DEBUG("Unregistered symbolic link: {} => {}", it->first, it->second);

  symlinks_.erase(it);
  return true;
}

bool VirtualFileSystem::FindSymbolicLink(const std::string_view path, std::string& target) {
  auto it = std::find_if(symlinks_.cbegin(), symlinks_.cend(), [&](const auto& s) {
    return rex::string::utf8_starts_with_case(path, s.first);
  });
  if (it == symlinks_.cend()) {
    return false;
  }
  target = (*it).second;
  return true;
}

bool VirtualFileSystem::ResolveSymbolicLink(const std::string_view path, std::string& result) {
  result = path;
  bool was_resolved = false;
  while (true) {
    auto it = std::find_if(symlinks_.cbegin(), symlinks_.cend(), [&](const auto& s) {
      return rex::string::utf8_starts_with_case(result, s.first);
    });
    if (it == symlinks_.cend()) {
      break;
    }
    // Found symlink!
    auto target_path = (*it).second;
    auto relative_path = result.substr((*it).first.size());
    result = target_path + relative_path;
    was_resolved = true;
  }
  return was_resolved;
}

Entry* VirtualFileSystem::ResolvePath(const std::string_view path) {
  auto global_lock = global_critical_region_.Acquire();

  // Resolve relative paths
  auto normalized_path(rex::string::utf8_canonicalize_guest_path(path));

  // Resolve symlinks.
  std::string resolved_path;
  bool had_symlink = ResolveSymbolicLink(normalized_path, resolved_path);
  if (had_symlink) {
    normalized_path = resolved_path;
  }

  // Find the device.
  auto it = std::find_if(devices_.cbegin(), devices_.cend(), [&](const auto& d) {
    return rex::string::utf8_starts_with_case(normalized_path, d->mount_path());
  });
  if (it == devices_.cend()) {
    REXFS_WARN("VFS: '{}' -> [no device]", path);
    // Supress logging the error for ShaderDumpxe:\CompareBackEnds as this is
    // not an actual problem nor something we care about.
    if (path != "ShaderDumpxe:\\CompareBackEnds") {
      REXFS_ERROR("ResolvePath({}) failed - device not found", path);
    }
    return nullptr;
  }

  const auto& device = *it;
  auto relative_path = normalized_path.substr(device->mount_path().size());
  auto* entry = device->ResolvePath(relative_path);

  if (entry) {
    if (had_symlink) {
      REXFS_TRACE("VFS resolved '{}' via symlink '{}' on device '{}' -> '{}'", path,
                  normalized_path, device->mount_path(), entry->absolute_path());
    } else {
      REXFS_TRACE("VFS resolved '{}' on device '{}' -> '{}'", path, device->mount_path(),
                  entry->absolute_path());
    }
  } else {
    if (had_symlink) {
      REXFS_WARN("VFS: entry not found for '{}' (via symlink '{}') on device '{}'", path,
                 normalized_path, device->mount_path());
    } else {
      REXFS_WARN("VFS: entry not found for '{}' on device '{}'", path, device->mount_path());
    }
  }

  return entry;
}

Entry* VirtualFileSystem::CreatePath(const std::string_view path, uint32_t attributes) {
  // Create all required directories recursively.
  auto path_parts = rex::string::utf8_split_path(path);
  if (path_parts.empty()) {
    return nullptr;
  }
  auto partial_path = std::string(path_parts[0]);
  auto partial_entry = ResolvePath(partial_path);
  if (!partial_entry) {
    return nullptr;
  }
  auto parent_entry = partial_entry;
  for (size_t i = 1; i < path_parts.size() - 1; ++i) {
    partial_path = rex::string::utf8_join_guest_paths(partial_path, path_parts[i]);
    auto child_entry = ResolvePath(partial_path);
    if (!child_entry) {
      child_entry = parent_entry->CreateEntry(path_parts[i], kFileAttributeDirectory);
    }
    if (!child_entry) {
      return nullptr;
    }
    parent_entry = child_entry;
  }
  return parent_entry->CreateEntry(path_parts[path_parts.size() - 1], attributes);
}

bool VirtualFileSystem::DeletePath(const std::string_view path) {
  auto entry = ResolvePath(path);
  if (!entry) {
    return false;
  }
  auto parent = entry->parent();
  if (!parent) {
    // Can't delete root.
    return false;
  }
  return parent->Delete(entry);
}

X_STATUS VirtualFileSystem::OpenFile(Entry* root_entry, const std::string_view path,
                                     FileDisposition creation_disposition, uint32_t desired_access,
                                     bool is_directory, bool is_non_directory, File** out_file,
                                     FileAction* out_action) {
  const uint32_t requested_access = desired_access;
  const auto trace_status = [&](X_STATUS status) {
#if REX_PLATFORM_PROSPERO
    auto& limit = XFAILED(status) ? vfs_error_logs : vfs_open_logs;
    if (limit.Allow(PROSPERO_GAME_VFS_OPEN)) {
      const auto hash = rex::prospero::diagnostics::PathHash(path);
      prospero_trace_checkpoint(PROSPERO_GAME_VFS_OPEN, hash, desired_access, status);
      rex::prospero::diagnostics::Line line("vfs_open");
      line.Number("path_hash", hash); line.Number("status", status);
      line.Number("requested_access", requested_access); line.Number("effective_access", desired_access);
      line.Number("disposition", uint32_t(creation_disposition));
      line.Number("directory", is_directory); line.Number("non_directory", is_non_directory);
      line.Number("suppressed_total", limit.total_suppressed());
      line.Path("path_pct", path);
      if (root_entry) line.Path("root_pct", root_entry->absolute_path());
      line.Write();
    }
#else
    (void)requested_access;
#endif
    return status;
  };
  // TODO(gibbed): should 'is_directory' remain as a bool or should it be
  // flipped to a generic FileAttributeFlags?

  // Cleanup access.
  if (desired_access & FileAccess::kGenericRead) {
    desired_access |= FileAccess::kFileReadData;
  }
  if (desired_access & FileAccess::kGenericWrite) {
    desired_access |= FileAccess::kFileWriteData;
  }
  if (desired_access & FileAccess::kGenericAll) {
    desired_access |= FileAccess::kFileReadData | FileAccess::kFileWriteData;
  }

  // Lookup host device/parent path.
  // If no device or parent, fail.
  Entry* parent_entry = nullptr;
  Entry* entry = nullptr;

  auto base_path = rex::string::utf8_find_base_guest_path(path);
  if (!base_path.empty()) {
    parent_entry = !root_entry ? ResolvePath(base_path) : root_entry->ResolvePath(base_path);
    if (!parent_entry) {
      *out_action = FileAction::kDoesNotExist;
      return trace_status(X_STATUS_NO_SUCH_FILE);
    }

    auto file_name = rex::string::utf8_find_name_from_guest_path(path);
    entry = parent_entry->GetChild(file_name);
  } else {
    entry = !root_entry ? ResolvePath(path) : root_entry->GetChild(path);
  }

  if (entry) {
    if (entry->attributes() & kFileAttributeDirectory && is_non_directory) {
      return trace_status(X_STATUS_FILE_IS_A_DIRECTORY);
    }

    // If the cached entry does not exist on host anymore, invalidate it.
    if (parent_entry) {
      const auto* host_path_entry = dynamic_cast<const HostPathEntry*>(parent_entry);
      if (host_path_entry) {
        const auto file_path = host_path_entry->host_path() / rex::to_path(entry->name());
        if (!std::filesystem::exists(file_path)) {
          entry->Delete();
          entry = nullptr;
        }
      }
    }
  }

  // Check if exists (if we need it to), or that it doesn't (if it shouldn't).
  switch (creation_disposition) {
    case FileDisposition::kOpen:
    case FileDisposition::kOverwrite:
      // Must exist.
      if (!entry) {
        *out_action = FileAction::kDoesNotExist;
        return trace_status(X_STATUS_NO_SUCH_FILE);
      }
      break;
    case FileDisposition::kCreate:
      // Must not exist.
      if (entry) {
        *out_action = FileAction::kExists;
        return trace_status(X_STATUS_OBJECT_NAME_COLLISION);
      }
      break;
    default:
      // Either way, ok.
      break;
  }

  // Verify permissions.
  bool wants_write =
      desired_access & FileAccess::kFileWriteData || desired_access & FileAccess::kFileAppendData;
  if (wants_write &&
      ((parent_entry && parent_entry->is_read_only()) || (entry && entry->is_read_only()))) {
    // Match Xenia behavior: downgrade to read access instead of failing.
    REXFS_WARN("Attempted to open read-only file/dir for write: {}", path);
    desired_access = FileAccess::kGenericRead | FileAccess::kFileReadData;
  }

  bool created = false;
  if (!entry) {
    // Remember that we are creating this new, instead of replacing.
    created = true;
    *out_action = FileAction::kCreated;
  } else {
    // May need to delete, if it exists.
    switch (creation_disposition) {
      case FileDisposition::kCreate:
        // Shouldn't be possible to hit this.
        assert_always();
        return trace_status(X_STATUS_ACCESS_DENIED);
      case FileDisposition::kSuperscede:
        // Replace (by delete + recreate).
        if (!entry->Delete()) {
          return trace_status(X_STATUS_ACCESS_DENIED);
        }
        entry = nullptr;
        *out_action = FileAction::kSuperseded;
        break;
      case FileDisposition::kOpen:
      case FileDisposition::kOpenIf:
        // Normal open.
        *out_action = FileAction::kOpened;
        break;
      case FileDisposition::kOverwrite:
      case FileDisposition::kOverwriteIf:
        // Overwrite by delete + recreate, or truncate if delete fails
        // (host file may be briefly locked by cloud sync, AV, etc.).
        if (entry->Delete()) {
          entry = nullptr;
        } else if (!entry->Truncate()) {
          return trace_status(X_STATUS_ACCESS_DENIED);
        }
        *out_action = FileAction::kOverwritten;
        break;
    }
  }
  if (!entry) {
    // Create if needed (either new or as a replacement).
    entry = CreatePath(path, !is_directory ? kFileAttributeNormal : kFileAttributeDirectory);
    if (!entry) {
      return trace_status(X_STATUS_ACCESS_DENIED);
    }
  }

  // Open.
  auto result = entry->Open(desired_access, out_file);
  if (XFAILED(result)) {
    *out_action = FileAction::kDoesNotExist;
  }
  return trace_status(result);
}

}  // namespace rex::filesystem
