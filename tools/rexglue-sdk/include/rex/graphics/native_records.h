#pragma once
/**
 * @file        native_records.h
 * @brief       Native rendering records produced by hooks on the game's Direct3D layer.
 *
 * The game's Direct3D code turns every draw's state into Xbox command packets
 * that the command processor then decodes register by register. A native hook
 * instead captures the state straight from the Direct3D device into a record
 * and leaves only a small marker packet in the command buffer, so records are
 * consumed in exactly the order the packets they replace would have been.
 *
 * Producer: the guest render thread (hooks). Consumer: the command processor.
 */

#include <cstddef>
#include <cstdint>
#include <vector>

namespace rex::graphics::native_records {

// PM4 type-3 opcode (unused by the Xenos) of the marker packet. Payload: the
// record's sequence number, then how many dwords of the original packets the
// record replaces follow the marker. With its record the command processor
// applies the record and skips those packets; without (a recorded command
// buffer replayed, or a record already dropped) it executes the packets, so
// command buffers the game records once and replays every frame stay correct.
constexpr uint32_t kMarkerOpcode = 0x7E;
constexpr uint32_t kMarkerPayloadDwords = 2;
constexpr uint32_t kMarkerHeader =
    0xC0000000u | ((kMarkerPayloadDwords - 1) << 16) | (kMarkerOpcode << 8);

enum class RecordType : uint32_t {
  // Runs of consecutive registers: {first register, count, count dwords of
  // values in guest (big-endian) byte order}, repeated.
  kRegisterRuns = 1,
};

enum RecordFlags : uint32_t {
  // The packets the record replaces were also written; the command processor
  // compares the registers they set against the record instead of applying it.
  kRecordFlagVerify = 1u << 0,
};

// Whether a command processor consumes records. Hooks must write their
// original packets when not.
bool Enabled();
void SetEnabled(bool enabled);

// Queues a record and returns its sequence number for the marker packet.
uint32_t Push(RecordType type, uint32_t flags, const uint32_t* payload, size_t payload_dwords);

struct Record {
  uint32_t sequence = 0;
  RecordType type = RecordType::kRegisterRuns;
  uint32_t flags = 0;
  std::vector<uint32_t> payload;
};

// Takes the record with the sequence number, dropping older ones (their
// markers were never reached or will be replayed with their packets).
// Returns false if it is not queued.
bool Pop(uint32_t sequence, Record& record_out);

// Drops all queued records (when the consumer resets).
void Clear();

// Sequence number of the oldest queued record, 0 if none (diagnostics).
uint32_t OldestSequence();

}  // namespace rex::graphics::native_records
