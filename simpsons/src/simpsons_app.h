// simpsons - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/rex_app.h>
#include <rex/runtime.h>

#if REX_PLATFORM_PROSPERO
#include "prospero_guest_support.h"

void ConfigureProsperoPaths(rex::PathConfig& paths);
#endif

namespace rex::memory {
class Memory;
}

// ink_outlines.cpp: patches the ink outline constants into the loaded image.
void ApplyInkOutlineOptions(rex::memory::Memory* memory);

namespace rex::ui {
class Window;
}

// freecam.cpp: the free camera's key binds and keyboard listener.
void InitFreecam(rex::ui::Window* window);
void ShutdownFreecam(rex::ui::Window* window);

// eye_shading.cpp: patches the characters' eye shading into the loaded image.
void ApplyEyeShadingOptions(rex::memory::Memory* memory);

// physics_step.cpp: sets the Havok step threshold in the loaded image.
void ApplyPhysicsStepOptions(rex::memory::Memory* memory);

class SimpsonsApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

#if REX_PLATFORM_PROSPERO
  std::string GetWindowTitle() const override { return "The Simpsons Game"; }

  void OnConfigurePaths(rex::PathConfig& paths) override {
    ConfigureProsperoPaths(paths);
  }
#endif

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<SimpsonsApp>(new SimpsonsApp(ctx, "simpsons",
        PPCImageConfig));
  }

  void OnPostLoadXexImage() override {
#if REX_PLATFORM_PROSPERO
    simpsons::InitializeProsperoGuestSupport(runtime()->memory());
#endif
    ApplyInkOutlineOptions(runtime()->memory());
    ApplyEyeShadingOptions(runtime()->memory());
    ApplyPhysicsStepOptions(runtime()->memory());
  }
  void OnPostSetup() override { InitFreecam(window()); }
  void OnShutdown() override { ShutdownFreecam(window()); }

  // Override virtual hooks for customization:
  // void OnPostInitLogging() override {}
  // void OnPreSetup(rex::RuntimeConfig& config) override {}
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnPostSetup() override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // void OnShutdown() override {}
  // void OnConfigurePaths(rex::PathConfig& paths) override {}
};
