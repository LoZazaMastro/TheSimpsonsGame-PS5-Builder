#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <map>
#include <memory>
#include <string>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ui/startup_prospero.h>
#include <rex/ui/windowed_app.h>
#include <rex/ui/windowed_app_context_prospero.h>

namespace {

void ReportFailure(const char* message, bool logging_ready) noexcept {
  // The CRT logger still works when TOML parsing, allocation, filesystem setup
  // or spdlog initialization failed. A read-only app0 can prevent its file from
  // being created; the kernel debug channel and post-CRT stderr remain useful.
  prospero_startup_log(message);
  std::fputs(message, stderr);
  std::fputc('\n', stderr);
  std::fflush(stderr);
  if (logging_ready) {
    try {
      REXLOG_ERROR("PS5 application failed: {}", message);
    } catch (...) {
      prospero_startup_log("logging: reporting the failure through ReXGlue also failed");
    }
  }
}

}  // namespace

// The native app CRT writes its own entry/constructor markers before calling
// this function. This first marker precedes all main-side allocation and the
// normal ReXGlue logger, distinguishing a loader/CRT failure from runtime boot.
extern "C" int main(int argc, char** argv) {
  prospero_startup_log("main: entered The Simpsons Game");

  int result = EXIT_FAILURE;
  bool logging_ready = false;
  bool initialization_started = false;
  std::unique_ptr<rex::ui::ProsperoWindowedAppContext> app_context;
  std::unique_ptr<rex::ui::WindowedApp> app;
  try {
    prospero_startup_log("main: initializing cvars and early ReXGlue logging");
    rex::InitLoggingEarly();
    logging_ready = true;
    auto remaining = rex::cvar::Init(argc, argv);
    rex::cvar::ApplyEnvironment();

    prospero_startup_log("main: constructing application context");
    app_context = std::make_unique<rex::ui::ProsperoWindowedAppContext>();
    const auto creator = rex::ui::GetWindowedAppCreator();
    if (!creator) {
      ReportFailure("main: no application creator was registered", logging_ready);
    } else {
      app = creator(*app_context);
      if (!app) {
        ReportFailure("main: application creation returned no object", logging_ready);
      } else {
        const auto& option_names = app->GetPositionalOptions();
        std::map<std::string, std::string> parsed;
        for (size_t i = 0; i < std::min(remaining.size(), option_names.size()); ++i) {
          parsed[option_names[i]] = remaining[i];
        }
        app->SetParsedArguments(std::move(parsed));
        prospero_startup_log("main: initializing application");
        initialization_started = true;
        if (app->OnInitialize()) {
          prospero_startup_log("main: entering UI event loop");
          app_context->RunMainLoop();
          result = EXIT_SUCCESS;
        } else {
          ReportFailure("main: application initialization failed; inspect the last runtime stage",
                        logging_ready);
        }
      }
    }
  } catch (const std::exception& error) {
    ReportFailure(error.what(), logging_ready);
  } catch (...) {
    ReportFailure("main: an unknown exception interrupted application startup or execution",
                  logging_ready);
  }

  // Stop queued callbacks before destroying the partially initialized app.
  // Initialization may have thrown before the context or app even existed.
  if (app_context) app_context->QuitFromUIThread();
  if (app && initialization_started) {
    prospero_startup_log("main: shutting down application");
    try {
      app->InvokeOnDestroy();
    } catch (const std::exception& error) {
      result = EXIT_FAILURE;
      ReportFailure(error.what(), logging_ready);
    } catch (...) {
      result = EXIT_FAILURE;
      ReportFailure("main: an unknown exception interrupted application shutdown", logging_ready);
    }
  }
  app.reset();
  app_context.reset();
  if (logging_ready) {
    try {
      rex::ShutdownLogging();
    } catch (const std::exception& error) {
      result = EXIT_FAILURE;
      ReportFailure(error.what(), false);
    } catch (...) {
      result = EXIT_FAILURE;
      ReportFailure("main: an unknown exception interrupted logging shutdown", false);
    }
  }
  prospero_startup_log(result == EXIT_SUCCESS ? "main: returning success" : "main: returning failure");
  return result;
}
