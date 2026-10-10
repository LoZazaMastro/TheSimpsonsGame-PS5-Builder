# Native WPF layout audit

This Windows-only harness instantiates the actual `Builder.Wpf` application,
compiled XAML resources, `MainWindow`, settings and dialogs. It does not build a
parallel HTML, WinForms or mock rendering of the interface. It never starts a
game-data build and does not need an ISO or `Lossless.dll`.

## Build

```powershell
dotnet publish prospero/builder/Builder.UiTests/Builder.UiTests.csproj -c Release -r win-x64 --self-contained true -o ui-audit
```

Keep the whole `ui-audit` folder. It contains the WPF runtime and production
assembly; the harness itself is not a single-file executable.

## Run on Windows

With Windows display scaling set to 100%, run:

```powershell
.\ui-audit\Builder.UiTests.exe C:\BuilderUiAudit\dpi96 96
```

Repeat in a new output directory at actual display scaling 125% (120 DPI), 150%
(144 DPI), and 200% (192 DPI). The optional DPI argument checks the real monitor
DPI and refuses to claim that a mismatched setting was tested. Changing output
bitmap resolution does not test a DPI transition, so the harness does not use
that shortcut. Multi-monitor movement must additionally be exercised manually.

At each real scaling, the runner traverses all four pages and every settings
category at 608×480, minimum, 1024×720 and 1220×840 logical window sizes, including
the bottom of long settings pages. The 608×480 case temporarily lowers the test
window's minimum dimensions to exercise its actual compact layout; it does not
change monitor DPI or alter the production executable. It also captures the folder-source choice, its
open popup where available in the WPF tree, maximize/restore, About and Colour.

Each capture includes actual runtime PNGs and resolved control/text bounds in
`layout-report.json`. Binding warnings, controls with no space, horizontal
window overflow and an inaccessible primary action are findings. Text metrics
can flag insufficient vertical space for review; they are not a visual verdict.
The report explicitly distinguishes `MEASURED_NO_FINDINGS`, `REVIEW_REQUIRED`,
`FAILED`, and `NOT_RUN`. Review the PNGs before claiming legibility passed.

`RenderTargetBitmap` does not capture the Windows desktop compositor's Acrylic
background, shadow or underlying desktop. Check those live on Windows, together
with mouse/keyboard interaction, dropdown scrolling, native file dialogs and
moving a window between displays with different scaling.

## Release gallery on Windows

The gallery mode captures three actual production windows at 1220×980 logical
pixels, using the real monitor DPI. It navigates to Game files, Graphics quality
and Experimental frame generation. The last view deliberately shows the normal
unavailable controls with no `Lossless.dll` selected. It also brings the in-game
shortcut hint into view. No ISO, game data, optional DLL, file-selection result
or successful build is fabricated.

After preparing the final payload in `Assets`, run this from the builder folder
on Windows with the .NET 10 SDK:

```powershell
.\capture-gallery.ps1 -WorkDirectory C:\BuilderGallery\v3.10.5 -ExpectedMonitorDpi 96
```

Use new work and destination directories so earlier evidence remains intact.
The default destination is `docs/screenshots` under the builder directory. An
optional `-DestinationDirectory` selects a different new directory. The script
does not install tools, change display scaling, contact GitHub or publish files.

The script publishes this harness with its project reference to `Builder.Wpf`,
then publishes `TheSimpsonsGame-PS5-Builder.exe` with `--no-build` from the same
compilation. It compares the production assembly used for publishing with the
assembly loaded by the harness, before and after capture. Any build failure,
binding warning, layout finding, missing image, changed assembly or hash mismatch
stops the script before a gallery can be accepted for publication.

The destination contains `source.png`, `graphics.png`, `frame-generation.png`
and `gallery-report.json`. The report includes the actual capture method, real
DPI and dimensions, SHA-256 of every PNG, the loaded production assembly and the
Windows executable built from it. The complete layout report, measurements,
runtime diagnostics and build logs stay in the work directory. Public metadata
omits local account paths. A relative-path SHA-256 inventory of the production
WPF, Core and harness source files, capture script and embedded notices/icon is
checked before and after building/capturing. The publication gate compares that
inventory again, so images from different source bytes cannot be reused.

For a manual harness run, preserving the original full audit mode:

```powershell
.\ui-audit\Builder.UiTests.exe --gallery C:\BuilderUiAudit\gallery 96
```

Before publishing documentation or a release, run the gate and stop if it fails:

```powershell
.\capture-gallery.ps1 -VerifyOnly -DestinationDirectory .\docs\screenshots
```

Pass `-ExpectedBuilderSha256` when publishing the same Windows-built executable
recorded in the gallery. The gate requires all three authentic PNGs and matching
hashes; a placeholder image or missing Windows run cannot satisfy it. Inspect
the three images before publication: `MEASURED_NO_FINDINGS` records successful
geometry and binding checks, and does not claim a human visual review or native
Acrylic desktop composition was tested.

## Verification performed in the delivery environment

The delivery environment is Linux. Its Wine server cannot initialize because
AF_UNIX socket creation is denied with errno 1 (`Operation not permitted`). No
native Windows execution or native WPF screenshots are claimed on that host.
Cross-compiling this project verifies C# and XAML build dependencies only.
