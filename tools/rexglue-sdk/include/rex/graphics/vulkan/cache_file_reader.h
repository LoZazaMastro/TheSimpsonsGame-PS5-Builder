// Bounded reads of reconstructible shader/pipeline caches. File positions are
// checked before allocation; a damaged record never supplies an allocation size.
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

#include <vulkan/vulkan.h>

namespace rex::graphics::vulkan::cache_file {

enum class ReadResult { kSuccess, kInvalidSize, kTruncated, kIoError };

// Shader programs submitted by both PM4 IM_LOAD variants have a 16-bit dword
// count (CommandProcessor::ExecutePacketType3_IM_LOAD[_IMMEDIATE]). The cache
// uses 31 bits, so its field must not be trusted as a host allocation request.
inline constexpr uint32_t kMaxShaderDwords = 0xFFFF;
// These are optional cache inputs, not limits on game resources. Oversized
// cache files are skipped intact; the normal runtime can recreate their data.
inline constexpr uint64_t kMaxShareableCacheBytes = uint64_t(64) << 20;
inline constexpr uint64_t kMaxDriverCacheBytes = uint64_t(256) << 20;
inline constexpr uint64_t kMaxShaderModuleBytes = uint64_t(16) << 20;

inline bool GetFileExtent(FILE* file, uint64_t& extent) {
  const long position = std::ftell(file);
  if (position < 0 || std::fseek(file, 0, SEEK_END) != 0) return false;
  const long end = std::ftell(file);
  if (std::fseek(file, position, SEEK_SET) != 0 || end < 0) return false;
  extent = uint64_t(end);
  return true;
}

template <typename T, typename Allocator>
ReadResult ReadElements(FILE* file, uint64_t file_extent, uint64_t count,
                        uint64_t max_count, std::vector<T, Allocator>& output) {
  const long position = std::ftell(file);
  if (position < 0 || std::ferror(file)) return ReadResult::kIoError;
  if (count > max_count || count > output.max_size() ||
      count > std::numeric_limits<size_t>::max() / sizeof(T)) {
    return ReadResult::kInvalidSize;
  }
  const uint64_t byte_count = count * sizeof(T);
  if (uint64_t(position) > file_extent || byte_count > file_extent - uint64_t(position)) {
    return ReadResult::kTruncated;
  }
  // Only a checked, bounded count reaches the allocation.
  output.resize(size_t(count));
  if (!byte_count) return ReadResult::kSuccess;
  if (std::fread(output.data(), 1, size_t(byte_count), file) != byte_count) {
    output.clear();
    return std::ferror(file) ? ReadResult::kIoError : ReadResult::kTruncated;
  }
  return ReadResult::kSuccess;
}

inline ReadResult ReadBlob(FILE* file, uint64_t maximum_bytes, std::vector<uint8_t>& output) {
  output.clear();
  uint64_t extent;
  if (!GetFileExtent(file, extent) || std::fseek(file, 0, SEEK_SET) != 0) {
    return ReadResult::kIoError;
  }
  return ReadElements(file, extent, extent, maximum_bytes, output);
}

inline bool DriverHeaderMatches(const std::vector<uint8_t>& data,
                                const VkPhysicalDeviceProperties& properties) {
  if (data.size() < sizeof(VkPipelineCacheHeaderVersionOne)) return false;
  VkPipelineCacheHeaderVersionOne header;
  std::memcpy(&header, data.data(), sizeof(header));
  return header.headerSize >= sizeof(header) && header.headerSize <= data.size() &&
         header.headerVersion == VK_PIPELINE_CACHE_HEADER_VERSION_ONE &&
         header.vendorID == properties.vendorID && header.deviceID == properties.deviceID &&
         std::memcmp(header.pipelineCacheUUID, properties.pipelineCacheUUID, VK_UUID_SIZE) == 0;
}

}  // namespace rex::graphics::vulkan::cache_file
