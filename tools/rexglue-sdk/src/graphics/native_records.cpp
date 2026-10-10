/**
 * @file        native_records.cpp
 * @brief       Native rendering records produced by hooks on the game's Direct3D layer.
 */

#include <rex/graphics/native_records.h>

#include <atomic>
#include <deque>
#include <mutex>

namespace rex::graphics::native_records {

namespace {
std::atomic<bool> g_enabled{false};
std::mutex g_mutex;
std::deque<Record> g_records;
uint32_t g_next_sequence = 1;
}  // namespace

bool Enabled() { return g_enabled.load(std::memory_order_acquire); }

void SetEnabled(bool enabled) { g_enabled.store(enabled, std::memory_order_release); }

uint32_t Push(RecordType type, uint32_t flags, const uint32_t* payload, size_t payload_dwords) {
  std::lock_guard<std::mutex> lock(g_mutex);
  Record& record = g_records.emplace_back();
  record.sequence = g_next_sequence++;
  if (!record.sequence) {
    // Keep 0 free as "no record".
    record.sequence = g_next_sequence++;
  }
  record.type = type;
  record.flags = flags;
  record.payload.assign(payload, payload + payload_dwords);
  return record.sequence;
}

bool Pop(uint32_t sequence, Record& record_out) {
  std::lock_guard<std::mutex> lock(g_mutex);
  while (!g_records.empty() && int32_t(g_records.front().sequence - sequence) < 0) {
    g_records.pop_front();
  }
  if (g_records.empty() || g_records.front().sequence != sequence) {
    return false;
  }
  record_out = std::move(g_records.front());
  g_records.pop_front();
  return true;
}

uint32_t OldestSequence() {
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_records.empty() ? 0 : g_records.front().sequence;
}

void Clear() {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_records.clear();
}

}  // namespace rex::graphics::native_records
