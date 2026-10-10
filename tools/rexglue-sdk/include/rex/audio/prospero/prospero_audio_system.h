// BSD 3-Clause License. See the project LICENSE.
#pragma once

#include <rex/audio/audio_system.h>

namespace rex::audio::prospero {

class ProsperoAudioSystem final : public AudioSystem {
 public:
  explicit ProsperoAudioSystem(runtime::FunctionDispatcher* function_dispatcher);
  ~ProsperoAudioSystem() override;

  static bool IsAvailable();
  static std::unique_ptr<AudioSystem> Create(runtime::FunctionDispatcher* function_dispatcher);

  X_STATUS CreateDriver(size_t index, rex::thread::Semaphore* semaphore,
                        AudioDriver** out_driver) override;
  void DestroyDriver(AudioDriver* driver) override;
};

}  // namespace rex::audio::prospero
