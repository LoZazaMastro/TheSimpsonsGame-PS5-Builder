// Tests the actual production atomic mailbox and persistence implementation.
#include <rex/platform/settings_prospero.h>
#include <rex/platform/settings_store_prospero.h>
#include <toml++/toml.hpp>
#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cerrno>
#include <iostream>
#include <iterator>
#include <sys/stat.h>
#include <thread>
#include <vector>

using namespace rex::platform::prospero;
namespace fs = std::filesystem;
// Linker interposition targets the real production fsync boundary. No test
// branch is compiled into the persistence implementation.
std::atomic<int> fsync_fault{0};
std::string external_path, external_bytes;
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int descriptor) {
  struct stat info{};
  assert(fstat(descriptor, &info) == 0);
  const int fault = fsync_fault.load();
  if ((fault == 2 && S_ISDIR(info.st_mode)) || (fault == 3 && S_ISREG(info.st_mode))) {
    fsync_fault = 0; errno = EIO; return -1;
  }
  const int result = __real_fsync(descriptor);
  if (fault == 1 && S_ISREG(info.st_mode)) {
    fsync_fault = 0;
    std::ofstream output(external_path); output << external_bytes; output.close();
    assert(output.good());
  }
  return result;
}
std::string ReadBytes(const fs::path& file) {
  std::ifstream input(file);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void Write(const fs::path& path, std::string_view bytes) {
  std::ofstream out(path); out << bytes; out.close(); assert(out.good());
}
int main(int argc, char** argv) {
  assert(argc == 2);
  LiveSettings live;
  const auto fx = LiveSetting::kSwapPostEffect;
  live.Initialize(fx, 0);
  assert(live.Requested(fx).version() == 1 && !live.Applied(fx).version());
  auto request = live.Request(fx, 1);
  assert(request.version() == 2 && live.Applied(fx).packed != request.packed);
  // The UI merely publishing cannot acknowledge its own change.
  live.Reject(fx, request);
  assert(live.Rejected(fx).packed == request.packed && live.Applied(fx).packed != request.packed);
  auto next = live.Request(fx, 2);
  live.Acknowledge(fx, next);
  live.Acknowledge(fx, request);  // Slow older consumer cannot regress the ack.
  assert(live.Applied(fx).packed == next.packed);
  assert(live.Rejected(fx).version() < live.Requested(fx).version());
  std::cout << "PASS requests stay pending until owner acknowledgement; explicit reject and stale-ack suppression\n";
  live.Initialize(fx, 0);
  live.Consume(fx);
  request = live.Request(fx, 1);
  live.CompleteEffect(fx, request, 0, true);  // gamma-only fallback still submitted successfully
  assert(live.Rejected(fx).packed == request.packed && live.Applied(fx).value() == 0);
  next = live.Request(fx, 2);
  live.CompleteEffect(fx, next, 2, false);  // matching shader, failed submission
  assert(live.Applied(fx).packed != next.packed);
  live.CompleteEffect(fx, next, 2, true);
  assert(live.Applied(fx).packed == next.packed);
  std::cout << "PASS submitted gamma-only fallback rejects FXAA; failed submission never acknowledges an effect\n";

  live.Initialize(LiveSetting::kFrameRates, LiveSettings::PackFrameRates(144, 0));
  assert(LiveSettings::GameFrameRate(live.Requested(LiveSetting::kFrameRates).value()) == 60);
  assert(LiveSettings::MenuFrameRate(live.Requested(LiveSetting::kFrameRates).value()) == 60);
  const auto valid_pair = live.Requested(LiveSetting::kFrameRates);
  for (int bad : {0, 1, 15, 29, 59, 90, 120, 144, 165, 240, 1000}) {
    assert(live.Request(LiveSetting::kFrameRates, LiveSettings::PackFrameRates(bad, 60)).packed == valid_pair.packed);
    assert(live.Request(LiveSetting::kFrameRates, LiveSettings::PackFrameRates(60, bad)).packed == valid_pair.packed);
  }
  std::cout << "PASS legacy native FPS normalize to 30/60; both gameplay/menu requests reject every unsupported rate\n";

  constexpr uint32_t changes = 50000;
  live.Initialize(LiveSetting::kFrameRates, LiveSettings::PackFrameRates(60, 30));
  std::atomic<bool> start{false}, done{false}, mismatch{false};
  std::atomic<uint64_t> observations{0};
  std::vector<std::thread> readers;
  for (int n = 0; n < 6; ++n) readers.emplace_back([&, n] {
    while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
    uint32_t previous_ack = 0;
    do {
      const auto pair = live.Requested(LiveSetting::kFrameRates);
      const auto game = LiveSettings::GameFrameRate(pair.value());
      const auto menu = LiveSettings::MenuFrameRate(pair.value());
      if (!((game == 60 && menu == 30) || (game == 30 && menu == 60))) mismatch = true;
      if ((pair.version() + n) % 23 == 0) std::this_thread::yield();
      live.Acknowledge(LiveSetting::kFrameRates, pair);
      const auto applied = live.Applied(LiveSetting::kFrameRates);
      if (applied.version() < previous_ack) mismatch = true;
      previous_ack = applied.version();
      ++observations;
    } while (!done.load(std::memory_order_acquire));
  });
  start.store(true, std::memory_order_release);
  for (uint32_t n = 0; n < changes; ++n)
    live.Request(LiveSetting::kFrameRates, (n & 1) ? LiveSettings::PackFrameRates(60, 30)
                                                 : LiveSettings::PackFrameRates(30, 60));
  done.store(true, std::memory_order_release);
  for (auto& reader : readers) reader.join();
  assert(!mismatch && observations > 0);
  live.Consume(LiveSetting::kFrameRates);
  assert(live.Applied(LiveSetting::kFrameRates).packed == live.Requested(LiveSetting::kFrameRates).packed);
  std::cout << "PASS six scheduler/GPU readers, " << changes << " updates, " << observations
            << " coherent observations, monotonically advancing acknowledgements\n";

  const fs::path directory = fs::absolute(argv[1]);
  fs::create_directories(directory);
  const auto config = directory / "simpsons.toml";
  Write(config, "prospero_profile_version = 34\nresolution_scale = 3\nframe_rate = 60\n"
                "custom_array = [1, 2, 3]\n[frame]\nrate = 90\n"
                "[custom_table]\nkeep = 'unchanged'\n");
  assert(chmod(config.c_str(), 0777) == 0);
  SaveSettingsValues(config, {{"frame_rate", int64_t(30)}, {"resolution_scale", int64_t(2)},
                              {"prospero_profile_version", int64_t(34)}, {"audio_mute", true}});
  auto loaded = ReadSettingsValues(config);
  assert(std::get<int64_t>(loaded.at("frame_rate")) == 30);
  assert(std::get<int64_t>(loaded.at("resolution_scale")) == 2);
  assert(std::get<int64_t>(loaded.at("prospero_profile_version")) == 34);
  assert(std::get<bool>(loaded.at("audio_mute")));
  const auto parsed = toml::parse_file(config.string());
  assert(parsed["custom_array"].as_array()->size() == 3);
  assert(parsed["custom_table"]["keep"].value<std::string>() == "unchanged");
  assert(!parsed["frame"]["rate"]);
  struct stat info{}; assert(stat(config.c_str(), &info) == 0 && (info.st_mode & 0777) == 0777);
  std::cout << "PASS flat/nested key normalization, unknown table/array preservation, profile34 and mode0777\n";

  // Closing the dialog does not consume any request. The app-owned writer
  // persists the desired value independently; destruction drains pending work.
  live.Initialize(fx, 0);
  const auto pending = live.Request(fx, 2);
  std::atomic<uint64_t> persisted{0}, observer_errors{0};
  {
    SettingsConfigStore store(config, [&](const auto& status) {
      if (!status.error.empty()) ++observer_errors;
      else persisted.store(status.persisted);
    });
    for (int64_t i = 0; i < 40; ++i)
      store.Queue({{"frame_rate", int64_t(30 + 30 * (i % 2))},
                   {"resolution_scale", int64_t(2)}, {"swap_post_effect", std::string("fxaa_extreme")},
                   {"prospero_profile_version", int64_t(34)}});
  }
  assert(live.Applied(fx).packed != pending.packed && persisted == 40 && !observer_errors);
  loaded = ReadSettingsValues(config);
  assert(std::get<int64_t>(loaded.at("frame_rate")) == 60);
  assert(std::get<std::string>(loaded.at("swap_post_effect")) == "fxaa_extreme");
  assert(std::get<int64_t>(loaded.at("resolution_scale")) == 2);
  assert(std::get<int64_t>(loaded.at("prospero_profile_version")) == 34);
  std::cout << "PASS closing before owner consumption still saves final desired live/restart values for next launch\n";

  SaveSettingsValues(config, {{"prospero_profile_version", int64_t(48)}});
  SaveSettingsValues(config, {{"prospero_profile_version", int64_t(34)}, {"frame_rate", int64_t(60)}});
  assert(std::get<int64_t>(ReadSettingsValues(config).at("prospero_profile_version")) == 48);
  const auto before_external_edit = ReadBytes(config);
  external_path = config.string();
  external_bytes = before_external_edit + "\nexternal_edit = 'preserve me'\n";
  fsync_fault = 1;
  bool conflict = false;
  try { SaveSettingsValues(config, {{"frame_rate", int64_t(30)}}); }
  catch (const std::exception&) { conflict = true; }
  assert(conflict && ReadBytes(config) == external_bytes);
  SaveSettingsValues(config, {{"frame_rate", int64_t(30)}});
  assert(ReadBytes(config).find("preserve me") != std::string::npos);
  std::cout << "PASS future profile marker preserved; external edit during file flush wins and retry merges it\n";

  fsync_fault = 2;  // rename succeeds; the following directory flush fails
  const auto uncertain = SaveSettingsValues(config, {{"frame_rate", int64_t(60)}});
  assert(!uncertain.durability_warning.empty());
  assert(std::get<int64_t>(ReadSettingsValues(config).at("frame_rate")) == 60);
  const auto before_flush_failure = ReadBytes(config);
  fsync_fault = 3;  // staging-file flush fails before rename
  bool flush_failed = false;
  try { SaveSettingsValues(config, {{"frame_rate", int64_t(30)}}); }
  catch (const std::exception&) { flush_failed = true; }
  assert(flush_failed && ReadBytes(config) == before_flush_failure);
  std::cout << "PASS post-rename durability warning is distinct from pre-rename save failure\n";

  const auto corrupt = directory / "corrupt.toml";
  Write(corrupt, "resolution_scale = [unterminated\n");
  const auto corrupt_before = ReadBytes(corrupt);
  bool rejected = false;
  try { SaveSettingsValues(corrupt, {{"frame_rate", int64_t(60)}}); }
  catch (const std::exception&) { rejected = true; }
  assert(rejected && ReadBytes(corrupt) == corrupt_before);
  const auto link = directory / "link.toml";
  fs::remove(link);
  fs::create_symlink(config.filename(), link);
  const auto target_before = ReadBytes(config);
  rejected = false;
  try { SaveSettingsValues(link, {{"frame_rate", int64_t(30)}}); }
  catch (const std::exception&) { rejected = true; }
  assert(rejected && fs::is_symlink(link) && ReadBytes(config) == target_before);
  {
    SettingsConfigStore store(corrupt, [&](const auto& status) {
      if (!status.error.empty()) ++observer_errors;
    });
    store.Queue({{"frame_rate", int64_t(30)}});
  }
  assert(observer_errors == 1 && ReadBytes(corrupt) == corrupt_before);
  std::cout << "PASS failed synchronous/asynchronous save preserves original bytes and surfaces error\n";

  std::atomic<bool> writing{true}, partial{false};
  std::thread reader([&] {
    while (writing.load(std::memory_order_acquire)) {
      try {
        const auto observed = toml::parse_file(config.string());
        const auto value = observed["frame_rate"].value<int64_t>();
        if (!value || (*value != 30 && *value != 60)) partial = true;
        if (observed["custom_array"].as_array()->size() != 3) partial = true;
      } catch (...) { partial = true; }
    }
  });
  for (int64_t i = 0; i < 30; ++i)
    SaveSettingsValues(config, {{"frame_rate", int64_t(i % 2 ? 60 : 30)}});
  writing.store(false, std::memory_order_release);
  reader.join();
  assert(!partial);
  for (const auto& entry : fs::directory_iterator(directory))
    assert(entry.path().filename().string().find(".settings-") == std::string::npos);
  std::cout << "PASS readers see complete TOML during 30 durable replacements; no temporary file leaks\n";
}
