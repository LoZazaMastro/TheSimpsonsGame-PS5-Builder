// Subtitles setting.
//
// The game keeps its subtitle option in bit 0 of a flags word in its settings
// object and reads it through sub_823A0C08(settings, 0) wherever subtitles
// can be drawn. The option starts off and can only be turned on in the
// options menu, which a new game's first cutscene plays before (issue #27).
// With `subtitles` on, that read always reports subtitles as enabled.

#include <rex/cvar.h>
#include <rex/ppc.h>
#if REX_PLATFORM_PROSPERO
#include <rex/platform/settings_prospero.h>
#endif

REXCVAR_DEFINE_BOOL(subtitles, false, "Game",
                    "Always show subtitles, including the first cutscene of a new game "
                    "(overrides the in-game setting)");

// Returns bit r4 (0 to 2) of the settings flags at r3 + 8; bit 0 is subtitles.
REX_EXTERN(__imp__sub_823A0C08);
REX_EXTERN(sub_823A0C08);

REX_FUNC(sub_823A0C08) {
  if (ctx.r4.u32 == 0 &&
#if REX_PLATFORM_PROSPERO
      rex::platform::prospero::GetLiveSettings().Consume(
          rex::platform::prospero::LiveSetting::kSubtitles)
#else
      REXCVAR_GET(subtitles)
#endif
      ) {
    ctx.r3.u64 = 1;
    return;
  }
  __imp__sub_823A0C08(ctx, base);
}
