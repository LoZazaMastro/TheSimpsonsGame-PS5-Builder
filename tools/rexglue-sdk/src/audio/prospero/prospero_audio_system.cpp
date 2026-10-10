// BSD 3-Clause License. See the project LICENSE.
#include <rex/audio/prospero/prospero_audio_driver.h>
#include <rex/audio/prospero/prospero_audio_system.h>
#include <rex/platform.h>

#if !REX_PLATFORM_PROSPERO
#error The Prospero audio backend requires a native PS5 build.
#endif

namespace rex::audio::prospero {

ProsperoAudioSystem::ProsperoAudioSystem(runtime::FunctionDispatcher* dispatcher)
    : AudioSystem(dispatcher) {}

ProsperoAudioSystem::~ProsperoAudioSystem() {
  Shutdown();
}

bool ProsperoAudioSystem::IsAvailable() {
  // Backend availability is a build property. Hardware/device failures are
  // reported by CreateDriver instead of falling back to a silent NOP device.
  return true;
}

std::unique_ptr<AudioSystem> ProsperoAudioSystem::Create(
    runtime::FunctionDispatcher* dispatcher) {
  return std::make_unique<ProsperoAudioSystem>(dispatcher);
}

X_STATUS ProsperoAudioSystem::CreateDriver(size_t, rex::thread::Semaphore* semaphore,
                                          AudioDriver** out_driver) {
  if (!out_driver || !semaphore) {
    return X_STATUS_INVALID_PARAMETER;
  }
  *out_driver = nullptr;
  auto driver = std::make_unique<ProsperoAudioDriver>(memory_, semaphore);
  if (!driver->Initialize()) {
    return X_STATUS_UNSUCCESSFUL;
  }
  *out_driver = driver.release();
  return X_STATUS_SUCCESS;
}

void ProsperoAudioSystem::DestroyDriver(AudioDriver* driver) {
  delete static_cast<ProsperoAudioDriver*>(driver);
}

}  // namespace rex::audio::prospero
