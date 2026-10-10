using System.Diagnostics;
using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using Simpsons.Builder;
using Simpsons.Builder.Tests;

internal static class Program
{
    private static readonly List<object> Results = [];
    private static readonly List<object> Comparisons = [];
    private static string work = "";
    private static Capabilities capabilities = null!;
    private static BundlePackage bundle = null!;
    private static int sequence;
    private static string NewPath(string label) => Path.Combine(work, $"{++sequence:000}-{label}");
    private static Dictionary<string, JsonElement> Defaults(bool losslessAvailable = false) => capabilities.Defaults(losslessAvailable);
    private static BuildRequest Request(string input, string output, InputKind kind = InputKind.Iso, GamePatchOptions? options = null) => new(new(kind, input), output, Defaults(), options);
    private static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    private static string Sha(byte[] bytes) => Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant();

    private static async Task<int> Main(string[] args)
    {
        if (args.Length < 3)
        {
            Console.Error.WriteLine("Usage: Builder.Tests <capabilities.json> <work-parent> <report.json> [extract-xiso] [real-bundle-assets]"); return 2;
        }
        work = Path.Combine(Path.GetFullPath(args[1]), "builder-tests-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(work);
        byte[] capabilityBytes = File.ReadAllBytes(args[0]);
        using (Stream stream = new MemoryStream(capabilityBytes, writable: false)) capabilities = JsonFiles.Read<Capabilities>(stream);
        bundle = Fixtures.Bundle(capabilities).Bundle;
        int exitCode = 0;
        try
        {
            await Run("capabilities, typed defaults, all choices and presets", () =>
            {
                capabilities.ValidateManifest();
                foreach (CapabilitySetting setting in capabilities.Settings)
                    foreach (CapabilityChoice choice in setting.Choices)
                    {
                        var values = Defaults(); values[setting.Key] = choice.Value; capabilities.ValidateValues(values);
                    }
                foreach (CapabilityPreset preset in capabilities.Presets)
                {
                    var values = Defaults(); foreach ((string key, JsonElement value) in preset.Values) values[key] = value; capabilities.ValidateValues(values);
                }
                string toml = capabilities.WriteToml(Defaults(losslessAvailable: true));
                const string requestedDefaultSettings = """
                    fullscreen = true
                    gpu = "vulkan"
                    gpu_memory_mirror_on_demand = false
                    native_resolve_composed_reads = false
                    native_resolve_fresh_initial = false
                    native_resolve_surface_aliases = false
                    prospero_profile_version = 34
                    render_target_path_vulkan = "native"
                    vulkan_allow_present_mode_fifo_relaxed = false
                    vulkan_allow_present_mode_immediate = false
                    vulkan_device = -1
                    resolution = "4k"
                    prospero_output_hz = 120
                    vsync = false
                    present_letterbox = true
                    resolution_scale = 3
                    native_rt_original_resolution_targets = ""
                    present_effect = "fsr"
                    present_fsr_sharpness_reduction = 0.0
                    swap_post_effect = "fxaa"
                    eye_shading = "clean"
                    anisotropic_override = 5
                    character_tessellation = "off"
                    ink_outlines = "original"
                    ink_outline_strength = 0.6
                    ink_outline_color = "000000"
                    color_vibrance = 0.0
                    color_saturation = 1.0
                    color_contrast = 1.0
                    color_brightness = 0.0
                    frame_rate = 60
                    menu_frame_rate = 30
                    user_language = 1
                    frame_gen = true
                    frame_gen_target_rate = 120
                    frame_gen_multiplier = 2
                    subtitles = false
                    audio_mute = false
                    audio_maxqframes = 8
                    """;
                string[] actualSettings = toml.Split('\n', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries)
                    .Where(line => !line.StartsWith('#')).Order(StringComparer.Ordinal).ToArray();
                string[] expectedSettings = requestedDefaultSettings.Split('\n', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries)
                    .Order(StringComparer.Ordinal).ToArray();
                Check(actualSettings.SequenceEqual(expectedSettings), "The complete serialized PS5 startup preset changed. Actual TOML:\n" + toml);
                foreach (string key in new[] { "frame_rate", "menu_frame_rate" })
                    Check(capabilities.Settings.Single(s => s.Key == key).Choices.Select(c => c.Value.GetInt32()).Order().SequenceEqual(new[] { 30, 60 }), "Native FPS choices must be 30 and 60 only.");
                Check(capabilities.Settings.Where(s => s.Group == "framegen").Count() == 3 && capabilities.Groups.Single(g => g.Id == "framegen").RequiresFile == "lossless.dll", "Frame generation dependency is incomplete.");
                Check(capabilities.Settings.Where(s => s.Group == "framegen").All(s => s.Requires is null), "The supplied file must unlock all three frame generation choices.");
                foreach ((string key, int[] allowed) in new Dictionary<string, int[]>
                {
                    ["frame_gen_multiplier"] = [2, 3, 4],
                    ["frame_gen_target_rate"] = [0, 60, 120],
                    ["prospero_output_hz"] = [60, 120]
                })
                    Check(capabilities.Settings.Single(s => s.Key == key).Choices.Select(c => c.Value.GetInt32()).Order().SequenceEqual(allowed), "Unsupported frame generation or display choice: " + key);
                File.WriteAllText(Path.Combine(work, "recommended.toml"), toml);
                return Task.CompletedTask;
            });
            await Run("setting injection, wrong type, missing key, range and unknown option rejected", () =>
            {
                var bad = Defaults(); bad["user_language"] = JsonSerializer.SerializeToElement("6\nfullscreen = false"); Reject(() => capabilities.ValidateValues(bad));
                bad = Defaults(); bad["unrecognized"] = JsonSerializer.SerializeToElement(true); Reject(() => capabilities.ValidateValues(bad));
                bad = Defaults(); bad.Remove("frame_rate"); Reject(() => capabilities.ValidateValues(bad));
                bad = Defaults(); bad["resolution_scale"] = JsonSerializer.SerializeToElement(99); Reject(() => capabilities.ValidateValues(bad));
                bad = Defaults(); bad["ink_outline_strength"] = JsonSerializer.SerializeToElement(0.62); Reject(() => capabilities.ValidateValues(bad));
                Check(Capabilities.TomlValue(JsonSerializer.SerializeToElement("a\n\"b\\c")) == "\"a\\u000A\\\"b\\\\c\"", "TOML string escaping is incorrect.");
                return Task.CompletedTask;
            });
            await Run("unsafe Windows file names and traversal rejected", () =>
            {
                foreach (string name in new[] { "..", ".", "../escape", "a\\b", "C:relative", "NUL.txt", "COM1", "trailing.", "trailing ", "x\0y", "name:stream" }) Reject(() => SafePaths.ValidateComponent(name));
                Reject(() => SafePaths.ValidateRelative("/absolute")); Reject(() => SafePaths.ValidateRelative("root//empty"));
                return Task.CompletedTask;
            });

            await DiscIdentityChecks.RunAllAsync(Run, bundle, capabilities, NewPath);

            foreach (long partition in new long[] { 0, 0x0FD90000, 0x02080000, 0x18300000 })
            {
                long current = partition;
                await Run($"XDVDFS extraction + full pipeline at partition 0x{current:x}", async () =>
                {
                    IsoFixture fixture = Fixtures.Iso(NewPath("partition.iso"), current);
                    string output = NewPath("output");
                    BuildResult result = await new BuildPipeline(bundle).BuildAsync(Request(fixture.Path, output), null, null, CancellationToken.None);
                    VerifyGameBytes(Path.Combine(result.OutputDirectory, "gamedata"), Fixtures.GameFiles());
                    Check(result.XboxTitleId == "45410809" && result.DefaultXexSha256 == Sha(Fixtures.Xex()), "Identity or XEX hash incorrect.");
                    Check(!Directory.EnumerateDirectories(output, ".*.building-*").Any(), "Staging left behind after success.");
                    VerifyRecordedHashes(result.OutputDirectory);
                    if (args.Length >= 4) CompareUpstream(args[3], fixture.Path, Path.Combine(result.OutputDirectory, "gamedata"));
                });
            }
            await Run("64-bit ISO length beyond 4 GiB is handled without truncation", () =>
            {
                IsoFixture fixture = Fixtures.Iso(NewPath("large.iso"), 0x0FD90000);
                using (var stream = new FileStream(fixture.Path, FileMode.Open, FileAccess.Write)) stream.SetLength((1L << 32) + 12345);
                using GameSource source = GameSource.Open(new(InputKind.Iso, fixture.Path), "45410809", CancellationToken.None);
                Check(source.TotalBytes == Fixtures.GameFiles().Values.Sum(b => (long)b.Length), "Large image size corrupted the file sizes.");
                return Task.CompletedTask;
            });
            await Run("trimmed XGD3 image accepts valid lower candidate after an out-of-file probe", () =>
            {
                IsoFixture fixture = Fixtures.Iso(NewPath("trimmed-xgd3.iso"), 0x02080000);
                long lastByte = fixture.FileOffsets.Max(p => p.Value + Fixtures.GameFiles()[p.Key].Length);
                using (var stream = new FileStream(fixture.Path, FileMode.Open, FileAccess.Write)) stream.SetLength(lastByte);
                using GameSource source = GameSource.Open(new(InputKind.Iso, fixture.Path), "45410809", CancellationToken.None);
                Check(source.PartitionOffset == 0x02080000, "Lower valid partition was not found.");
                return Task.CompletedTask;
            });
            await Run("multi-sector AVL directory matches extract-xiso", async () =>
            {
                Dictionary<string, byte[]> files = Fixtures.GameFiles();
                for (int i = 0; i < 180; i++) files.Add($"extra/f_{i:000}.bin", [(byte)i]);
                IsoFixture fixture = Fixtures.Iso(NewPath("avl.iso"), files: files);
                BuildResult result = await new BuildPipeline(bundle).BuildAsync(Request(fixture.Path, NewPath("avl-output")), null, null, CancellationToken.None);
                VerifyGameBytes(Path.Combine(result.OutputDirectory, "gamedata"), files);
                if (args.Length >= 4) CompareUpstream(args[3], fixture.Path, Path.Combine(result.OutputDirectory, "gamedata"));
            });
            await Run("existing gamedata folder is copied and root case normalized", async () =>
            {
                string folder = Fixtures.Folder(NewPath("source-folder"));
                BuildResult result = await new BuildPipeline(bundle).BuildAsync(Request(folder, NewPath("folder-output"), InputKind.GameDataFolder), null, null, CancellationToken.None);
                Check(result.XboxTitleId == "45410809" && result.DefaultXexSha256 == Sha(Fixtures.Xex()), "Folder identity or XEX hash incorrect.");
                VerifyGameBytes(Path.Combine(result.OutputDirectory, "gamedata"), Fixtures.GameFiles());
                VerifyGameBytes(folder, Fixtures.GameFiles(), canonical: false);
            });
            await Run("source-folder modification after inspection detected", () =>
            {
                string folder = Fixtures.Folder(NewPath("changed-source"));
                using GameSource source = GameSource.Open(new(InputKind.GameDataFolder, folder), "45410809", CancellationToken.None);
                GameEntry file = source.Entries.First(e => e.RelativePath == "archive.bin");
                File.AppendAllText(file.SourcePath!, "changed"); Reject(() => source.OpenFile(file).Dispose());
                return Task.CompletedTask;
            });
            await Run("wrong Xbox title and malformed XEX headers rejected", () =>
            {
                var wrong = Fixtures.GameFiles(); wrong["DEFAULT.XEX"] = Fixtures.Xex(0x12345678);
                RejectSource(Fixtures.Iso(NewPath("wrong-game.iso"), files: wrong).Path);
                var magic = Fixtures.GameFiles(); magic["DEFAULT.XEX"][0] = 0; RejectSource(Fixtures.Iso(NewPath("bad-xex.iso"), files: magic).Path);
                var offset = Fixtures.GameFiles(); offset["DEFAULT.XEX"][28] = 0xFF; RejectSource(Fixtures.Iso(NewPath("bad-xex-offset.iso"), files: offset).Path);
                foreach (uint execution in new uint[] { 28, 105 })
                {
                    offset = Fixtures.GameFiles(); BinaryPrimitives.WriteUInt32BigEndian(offset["DEFAULT.XEX"].AsSpan(28), execution);
                    RejectSource(Fixtures.Iso(NewPath("xex-execution-bounds.iso"), files: offset).Path);
                }
                return Task.CompletedTask;
            });
            await Run("missing movies, empty movies and missing default.xex rejected", () =>
            {
                var files = Fixtures.GameFiles(); foreach (string key in files.Keys.Where(k => k.StartsWith("MOVIES/")).ToList()) files.Remove(key);
                RejectSource(Fixtures.Iso(NewPath("no-movies.iso"), files: files).Path);
                string empty = Fixtures.Folder(NewPath("empty-movies"), files); Directory.CreateDirectory(Path.Combine(empty, "movies"));
                Reject(() => GameSource.Open(new(InputKind.GameDataFolder, empty), "45410809", CancellationToken.None).Dispose());
                files = Fixtures.GameFiles(); files.Remove("DEFAULT.XEX"); RejectSource(Fixtures.Iso(NewPath("no-xex.iso"), files: files).Path);
                return Task.CompletedTask;
            });
            await Run("bad descriptor, truncated image and extent overflow rejected", () =>
            {
                IsoFixture fixture = Fixtures.Iso(NewPath("bad-magic.iso")); Fixtures.WriteAt(fixture.Path, 32L * 2048 + 2028, [0]); RejectSource(fixture.Path);
                fixture = Fixtures.Iso(NewPath("truncated.iso")); using (var stream = new FileStream(fixture.Path, FileMode.Open, FileAccess.Write)) stream.SetLength(fixture.RootOffset + 20); RejectSource(fixture.Path);
                fixture = Fixtures.Iso(NewPath("extent.iso")); Fixtures.WriteAt(fixture.Path, fixture.Records["archive.bin"] + 8, Fixtures.Little32(uint.MaxValue)); RejectSource(fixture.Path);
                return Task.CompletedTask;
            });
            await Run("AVL cycles, pointers outside table and directory recursion rejected", () =>
            {
                IsoFixture fixture = Fixtures.Iso(NewPath("pointer.iso")); Fixtures.WriteAt(fixture.Path, fixture.RootOffset, [0xFE, 0x3F]); RejectSource(fixture.Path);
                fixture = Fixtures.Iso(NewPath("cycle.iso")); long record = fixture.Records["DEFAULT.XEX"]; ushort self = (ushort)((record - fixture.RootOffset) / 4);
                Check(self != 0, "Cycle fixture must use non-root entry."); Fixtures.WriteAt(fixture.Path, record + 2, [(byte)self, (byte)(self >> 8)]); RejectSource(fixture.Path);
                fixture = Fixtures.Iso(NewPath("recursive.iso")); Fixtures.WriteAt(fixture.Path, fixture.Records["MOVIES"] + 4, Fixtures.Little32((uint)(fixture.RootOffset / 2048))); RejectSource(fixture.Path);
                fixture = Fixtures.Iso(NewPath("overlap.iso")); Fixtures.WriteAt(fixture.Path, fixture.RootOffset, [1, 0]); RejectSource(fixture.Path);
                return Task.CompletedTask;
            });
            await Run("malicious ISO file names and case collisions rejected before writing", () =>
            {
                IsoFixture fixture = Fixtures.Iso(NewPath("traversal.iso")); Fixtures.WriteAt(fixture.Path, fixture.Records["archive.bin"] + 14, "../evil.dat"u8); RejectSource(fixture.Path);
                fixture = Fixtures.Iso(NewPath("collision.iso")); Fixtures.WriteAt(fixture.Path, fixture.Records["archive.bin"] + 14, "default.xex"u8); RejectSource(fixture.Path);
                return Task.CompletedTask;
            });
            await Run("existing output and saves preserved", async () =>
            {
                IsoFixture fixture = Fixtures.Iso(NewPath("existing.iso")); string output = NewPath("existing-output");
                string save = Path.Combine(output, "PPSA74207", "userdata", "sentinel-save.bin"); Directory.CreateDirectory(Path.GetDirectoryName(save)!); File.WriteAllText(save, "SAVE MUST SURVIVE");
                await RejectAsync(() => new BuildPipeline(bundle).BuildAsync(Request(fixture.Path, output), null, null, CancellationToken.None));
                Check(File.ReadAllText(save) == "SAVE MUST SURVIVE", "Existing save was changed.");
            });
            await Run("output inside source folder rejected", async () =>
            {
                string folder = Fixtures.Folder(NewPath("nested-source")); string output = Path.Combine(folder, "new-output");
                await RejectAsync(() => new BuildPipeline(bundle).BuildAsync(Request(folder, output, InputKind.GameDataFolder), null, null, CancellationToken.None));
                Check(!Directory.Exists(output), "Output appeared inside source folder.");
            });
            await Run("destination created just before publication is preserved", async () =>
            {
                IsoFixture fixture = Fixtures.Iso(NewPath("race.iso")); string output = NewPath("race-output");
                string sentinel = Path.Combine(output, "PPSA74207", "keep.txt");
                var progress = new InlineProgress(p =>
                {
                    if (p.Phase != "Finishing") return;
                    Directory.CreateDirectory(Path.GetDirectoryName(sentinel)!); File.WriteAllText(sentinel, "CONCURRENT OUTPUT");
                });
                await RejectAsync(() => new BuildPipeline(bundle).BuildAsync(Request(fixture.Path, output), progress, null, CancellationToken.None));
                Check(File.ReadAllText(sentinel) == "CONCURRENT OUTPUT", "New destination was overwritten.");
                Check(!Directory.EnumerateDirectories(output, ".*.building-*").Any(), "Failed publication left staging behind.");
            });
            await Run("insufficient disk space stops before staging", async () =>
            {
                IsoFixture fixture = Fixtures.Iso(NewPath("space.iso")); string output = NewPath("space-output");
                await RejectAsync(() => new BuildPipeline(bundle, _ => 0).BuildAsync(Request(fixture.Path, output), null, null, CancellationToken.None));
                Check(!Directory.Exists(output), "Disk-space failure created output.");
            });
            foreach (string phase in new[] { "before", "Extracting", "Verifying", "Finishing" })
            {
                string target = phase;
                await Run("cancellation at " + target + " removes temporary output", async () =>
                {
                    IsoFixture fixture = Fixtures.Iso(NewPath("cancel.iso")); string output = NewPath("cancel-output");
                    using var ct = new CancellationTokenSource(); if (target == "before") ct.Cancel();
                    var progress = new InlineProgress(p => { if (p.Phase == target) ct.Cancel(); });
                    try { await new BuildPipeline(bundle).BuildAsync(Request(fixture.Path, output), progress, null, ct.Token); throw new Exception("Cancellation was ignored."); }
                    catch (OperationCanceledException) { }
                    Check(!Directory.Exists(Path.Combine(output, "PPSA74207")), "Cancelled output published.");
                    Check(!Directory.Exists(output) || !Directory.EnumerateFileSystemEntries(output).Any(), "Cancelled staging remains.");
                    using GameSource source = GameSource.Open(new(InputKind.Iso, fixture.Path), "45410809", CancellationToken.None);
                });
            }
            await Run("read-back verification detects output corruption", async () =>
            {
                IsoFixture fixture = Fixtures.Iso(NewPath("verify.iso")); string output = NewPath("verify-output"); bool changed = false;
                var progress = new InlineProgress(p =>
                {
                    if (p.Phase != "Verifying" || changed) return; changed = true;
                    string stage = Directory.GetDirectories(output, ".*.building-*").Single();
                    Fixtures.WriteAt(Path.Combine(stage, "gamedata", "archive.bin"), 0, [0xAA]);
                });
                await RejectAsync(() => new BuildPipeline(bundle).BuildAsync(Request(fixture.Path, output), progress, null, CancellationToken.None));
                Check(changed && !Directory.GetFileSystemEntries(output).Any(), "Corruption test did not remove temporary output.");
            });
            foreach (string defect in new[] { "hash", "path", "duplicate", "metadata" })
            {
                string selected = defect;
                await Run("bundled payload " + selected + " defect rejected", async () =>
                {
                    BundlePackage bad = Fixtures.Bundle(capabilities, metadataTitle: selected == "metadata" ? "Wrong name" : null,
                        extraName: selected == "path" ? "../escape.bin" : null, badHash: selected == "hash", duplicate: selected == "duplicate").Bundle;
                    IsoFixture fixture = Fixtures.Iso(NewPath("payload.iso")); string output = NewPath("payload-output");
                    await RejectAsync(() => new BuildPipeline(bad).BuildAsync(Request(fixture.Path, output), null, null, CancellationToken.None));
                    Check(!Directory.Exists(output) || !Directory.GetFileSystemEntries(output).Any(), "Invalid payload published or leaked staging.");
                });
            }
            await Run("reversible game patches integrated after source verification", async () =>
            {
                IsoFixture fixture = Fixtures.Iso(NewPath("patch.iso")); string originalHash = BuildPipeline.HashFile(fixture.Path, CancellationToken.None);
                BuildResult result = await new BuildPipeline(bundle).BuildAsync(Request(fixture.Path, NewPath("patch-output"), options: new GamePatchOptions(true, "BARTMAN_BEGINS")), null, null, CancellationToken.None);
                string game = Path.Combine(result.OutputDirectory, "gamedata");
                Check(File.Exists(Path.Combine(game, "movies/italian/ealogo.vp6.disabled")), "Intro logo was not preserved as disabled.");
                Check(!File.Exists(Path.Combine(game, "movies/italian/ealogo.vp6")), "Active intro logo remains.");
                Check(File.ReadAllBytes(Path.Combine(game, "simpsons_gameflow.lua.original")).SequenceEqual(Fixtures.GameFiles()["simpsons_gameflow.lua"]), "Original Lua bytes were not preserved.");
                Check(originalHash == BuildPipeline.HashFile(fixture.Path, CancellationToken.None), "Source ISO changed during patches.");
                VerifyRecordedHashes(result.OutputDirectory);
            });
            await Run("frame generation needs a selected file; display refresh remains independent", async () =>
            {
                capabilities.ValidateOptionalFileChoices(Defaults(), false);
                // The recommended preset now enables frame generation. This
                // must still be rejected without the optional file even though
                // the selected value equals the manifest's default.
                Reject(() => capabilities.ValidateOptionalFileChoices(capabilities.Defaults(), false));
                capabilities.ValidateOptionalFileChoices(Defaults(losslessAvailable: true), true);
                var displayOnly = Defaults(); displayOnly["prospero_output_hz"] = JsonSerializer.SerializeToElement(120);
                capabilities.ValidateValues(displayOnly); capabilities.ValidateOptionalFileChoices(displayOnly, false);
                foreach ((string key, JsonElement value) in new Dictionary<string, JsonElement>
                {
                    ["frame_gen"] = JsonSerializer.SerializeToElement(true),
                    ["frame_gen_target_rate"] = JsonSerializer.SerializeToElement(0),
                    ["frame_gen_multiplier"] = JsonSerializer.SerializeToElement(4)
                })
                {
                    var values = Defaults(); values[key] = value;
                    capabilities.ValidateValues(values);
                    Reject(() => capabilities.ValidateOptionalFileChoices(values, false));
                    capabilities.ValidateOptionalFileChoices(values, true);
                }
                IsoFixture fixture = Fixtures.Iso(NewPath("no-library.iso"));
                string output = NewPath("no-library-output"); var enabled = Defaults(); enabled["frame_gen"] = JsonSerializer.SerializeToElement(true);
                await RejectAsync(() => new BuildPipeline(bundle).BuildAsync(new(new(InputKind.Iso, fixture.Path), output, enabled), null, null, CancellationToken.None));
                Check(!Directory.Exists(output), "Missing optional library created staging.");
                BuildResult result = await new BuildPipeline(bundle).BuildAsync(new(new(InputKind.Iso, fixture.Path), output, displayOnly), null, null, CancellationToken.None);
                Check(!File.Exists(SafePaths.Under(result.OutputDirectory, OptionalLibrary.Destination)), "The default build unexpectedly bundled a DLL.");
                using JsonDocument info = JsonDocument.Parse(File.ReadAllText(Path.Combine(result.OutputDirectory, "build-info.json")));
                Check(!info.RootElement.GetProperty("losslessDllIncluded").GetBoolean(), "Absent DLL was reported as included.");
                Check(File.ReadAllText(Path.Combine(result.OutputDirectory, "userdata", "simpsons.toml")).Contains("frame_gen = false"), "A build without the optional file enabled frame generation.");
            });
            await Run("frame generation preset follows file selection, removal and reset without overriding manual Off on replacement", () =>
            {
                Dictionary<string, JsonElement> values = Defaults();
                Check(!values["frame_gen"].GetBoolean() && values["frame_gen_target_rate"].GetInt32() == 120 && values["frame_gen_multiplier"].GetInt32() == 2,
                    "The unavailable frame generation controls did not keep Off with the recommended target and multiplier.");
                capabilities.ApplyOptionalFileAvailability(values, wasAvailable: false, isAvailable: true);
                Check(values["frame_gen"].GetBoolean(), "Selecting the first optional file did not apply the recommended On switch.");
                Check(capabilities.ExperimentalSelections(values).Contains("Frame generation"), "Experimental frame generation disappeared from the summary because On became its default.");
                values["frame_gen"] = JsonSerializer.SerializeToElement(false);
                values["frame_gen_multiplier"] = JsonSerializer.SerializeToElement(4);
                capabilities.ApplyOptionalFileAvailability(values, wasAvailable: true, isAvailable: true);
                Check(!values["frame_gen"].GetBoolean() && values["frame_gen_multiplier"].GetInt32() == 4, "Replacing the optional file overwrote manual frame generation settings.");
                Check(!capabilities.ExperimentalSelections(values).Contains("Frame generation"), "An explicitly disabled frame generation switch was reported as enabled experimental use.");
                capabilities.ApplyOptionalFileAvailability(values, wasAvailable: true, isAvailable: false);
                capabilities.ValidateOptionalFileChoices(values, false);
                Check(!values["frame_gen"].GetBoolean() && values["frame_gen_multiplier"].GetInt32() == 2, "Removing the optional file retained enabled or non-default locked frame generation controls.");
                capabilities.ApplyOptionalFileAvailability(values, wasAvailable: false, isAvailable: true);
                Check(values["frame_gen"].GetBoolean(), "Reselecting the optional file did not apply the recommended switch.");
                Check(Defaults(losslessAvailable: true)["frame_gen"].GetBoolean() && !Defaults(losslessAvailable: false)["frame_gen"].GetBoolean(),
                    "Restore recommended settings ignored optional-file availability.");
                foreach (CapabilitySetting setting in capabilities.Settings.Where(s => s.Group != "framegen"))
                    Check(Capabilities.Equal(values[setting.Key], setting.Default), "Optional-file transitions changed an unrelated setting: " + setting.Key);
                return Task.CompletedTask;
            });
            await Run("arbitrary readable optional library is copied byte-exact without a format gate", async () =>
            {
                string library = NewPath("not-a-pe-arbitrary.data"); byte[] bytes = Enumerable.Range(0, 2 * 1024 * 1024 + 79).Select(i => (byte)(i * 37)).ToArray();
                File.WriteAllBytes(library, bytes);
                using (OptionalLibrary opened = OptionalLibrary.Open(library)) Check(opened.Length == bytes.Length, "Readable user file was rejected.");
                IsoFixture fixture = Fixtures.Iso(NewPath("library.iso")); var values = Defaults();
                values["frame_gen"] = JsonSerializer.SerializeToElement(true); values["frame_gen_target_rate"] = JsonSerializer.SerializeToElement(120);
                values["prospero_output_hz"] = JsonSerializer.SerializeToElement(120);
                BuildResult result = await new BuildPipeline(bundle).BuildAsync(new(new(InputKind.Iso, fixture.Path), NewPath("library-output"), values, LosslessDllPath: library), null, null, CancellationToken.None);
                Check(File.ReadAllBytes(SafePaths.Under(result.OutputDirectory, OptionalLibrary.Destination)).SequenceEqual(bytes), "DLL copy bytes or canonical case differ.");
                Check(File.ReadAllBytes(library).SequenceEqual(bytes), "User-supplied library changed.");
                VerifyRecordedHashes(result.OutputDirectory);
                using JsonDocument info = JsonDocument.Parse(File.ReadAllText(Path.Combine(result.OutputDirectory, "build-info.json")));
                Check(info.RootElement.GetProperty("losslessDllIncluded").GetBoolean() && info.RootElement.GetProperty("files").GetProperty(OptionalLibrary.Destination).GetString() == Sha(bytes), "DLL inclusion or integrity is not recorded.");
            });
            await Run("empty readable optional file is accepted and preserves a manually selected frame generation Off", async () =>
            {
                string library = NewPath("empty.dll"); File.WriteAllBytes(library, []);
                IsoFixture fixture = Fixtures.Iso(NewPath("empty-library.iso"));
                BuildResult result = await new BuildPipeline(bundle).BuildAsync(Request(fixture.Path, NewPath("empty-library-output")) with { LosslessDllPath = library }, null, null, CancellationToken.None);
                Check(new FileInfo(SafePaths.Under(result.OutputDirectory, OptionalLibrary.Destination)).Length == 0, "An unrequested content check rejected an empty readable file.");
                Check(File.ReadAllText(Path.Combine(result.OutputDirectory, "userdata", "simpsons.toml")).Contains("frame_gen = false"), "Supplying a file overrode the explicitly selected frame generation Off.");
                VerifyRecordedHashes(result.OutputDirectory);
            });
            await Run("missing, directory and inaccessible optional file stop before staging", async () =>
            {
                string missing = NewPath("missing.dll"); string directory = NewPath("directory.dll"); Directory.CreateDirectory(directory);
                foreach (string bad in new[] { missing, directory })
                {
                    Reject(() => OptionalLibrary.Open(bad).Dispose());
                    string output = NewPath("invalid-library-output"); IsoFixture fixture = Fixtures.Iso(NewPath("invalid-library.iso"));
                    await RejectAsync(() => new BuildPipeline(bundle).BuildAsync(Request(fixture.Path, output) with { LosslessDllPath = bad }, null, null, CancellationToken.None));
                    Check(!Directory.Exists(output), "An unavailable optional file created output.");
                }
                string locked = NewPath("inaccessible.dll"); File.WriteAllBytes(locked, [17, 23]);
                using var owner = new FileStream(locked, FileMode.Open, FileAccess.ReadWrite, FileShare.None);
                string lockedOutput = NewPath("locked-library-output"); IsoFixture lockedFixture = Fixtures.Iso(NewPath("locked-library.iso"));
                await RejectAsync(() => new BuildPipeline(bundle).BuildAsync(Request(lockedFixture.Path, lockedOutput) with { LosslessDllPath = locked }, null, null, CancellationToken.None));
                Check(!Directory.Exists(lockedOutput), "An inaccessible optional file created output.");
            });
            await Run("cancellation during optional library copy removes staging and preserves source", async () =>
            {
                string library = NewPath("cancel-library.dll"); byte[] bytes = new byte[3 * 1024 * 1024 + 13]; bytes[0] = 77; bytes[^1] = 91; File.WriteAllBytes(library, bytes);
                IsoFixture fixture = Fixtures.Iso(NewPath("cancel-library.iso")); string output = NewPath("cancel-library-output");
                using var cancel = new CancellationTokenSource(); long copied = 0;
                var progress = new InlineProgress(p => { if (p.CurrentFile == OptionalLibrary.Destination && p.BytesDone > 0) { copied = p.BytesDone; cancel.Cancel(); } });
                try { await new BuildPipeline(bundle).BuildAsync(Request(fixture.Path, output) with { LosslessDllPath = library }, progress, null, cancel.Token); throw new Exception("Library-copy cancellation was ignored."); }
                catch (OperationCanceledException) { }
                Check(copied > 0 && copied < bytes.Length, "Cancellation did not occur inside the copy.");
                Check(!Directory.GetFileSystemEntries(output).Any(), "Cancelled optional copy left staging behind.");
                Check(File.ReadAllBytes(library).SequenceEqual(bytes), "Cancelled copy changed the user file.");
            });
            await Run("optional library read-back corruption is detected", async () =>
            {
                string library = NewPath("corrupt-library.dll"); File.WriteAllBytes(library, [11, 22, 33, 44]);
                IsoFixture fixture = Fixtures.Iso(NewPath("corrupt-library.iso")); string output = NewPath("corrupt-library-output"); bool changed = false;
                var progress = new InlineProgress(p =>
                {
                    if (p.Phase != "Verifying" || changed) return;
                    changed = true; string stage = Directory.GetDirectories(output, ".*.building-*").Single();
                    Fixtures.WriteAt(SafePaths.Under(stage, OptionalLibrary.Destination), 0, [255]);
                });
                await RejectAsync(() => new BuildPipeline(bundle).BuildAsync(Request(fixture.Path, output) with { LosslessDllPath = library }, progress, null, CancellationToken.None));
                Check(changed && !Directory.GetFileSystemEntries(output).Any(), "Corrupt DLL escaped read-back verification or cleanup.");
                Check(File.ReadAllBytes(library).SequenceEqual(new byte[] { 11, 22, 33, 44 }), "Verification failure changed the source DLL.");
            });
            await Run("disk-space estimate includes the user-supplied optional library", async () =>
            {
                string library = NewPath("space-library.dll"); File.WriteAllBytes(library, new byte[4096]);
                IsoFixture fixture = Fixtures.Iso(NewPath("space-library.iso")); string output = NewPath("space-library-output");
                long enoughWithoutLibrary = Fixtures.GameFiles().Values.Sum(v => (long)v.Length) + bundle.Manifest.TotalBytes + 256L * 1024 * 1024;
                await RejectAsync(() => new BuildPipeline(bundle, _ => enoughWithoutLibrary).BuildAsync(Request(fixture.Path, output) with { LosslessDllPath = library }, null, null, CancellationToken.None));
                Check(!Directory.Exists(output), "Insufficient space for the optional DLL was ignored.");
            });
            if (!OperatingSystem.IsWindows())
            {
                await Run("source and output symbolic links rejected", async () =>
                {
                    string source = Fixtures.Folder(NewPath("link-source")); string link = NewPath("source-link"); Directory.CreateSymbolicLink(link, source);
                    Reject(() => GameSource.Open(new(InputKind.GameDataFolder, link), "45410809", CancellationToken.None).Dispose());
                    string output = NewPath("real-output"); Directory.CreateDirectory(output); string outputLink = NewPath("output-link"); Directory.CreateSymbolicLink(outputLink, output);
                    await RejectAsync(() => new BuildPipeline(bundle).BuildAsync(Request(source, outputLink, InputKind.GameDataFolder), null, null, CancellationToken.None));
                    Check(!Directory.GetFileSystemEntries(output).Any(), "Output symlink target changed.");
                    string library = NewPath("real-library.dll"); File.WriteAllBytes(library, [1, 2, 3]);
                    string libraryLink = NewPath("link-library.dll"); File.CreateSymbolicLink(libraryLink, library);
                    Reject(() => OptionalLibrary.Open(libraryLink).Dispose());
                });
            }
            await DiagnosticChecks.RunAllAsync(Run, bundle, capabilities, NewPath);
            if (args.Length >= 5)
            {
                await Run("actual bundled PS5 title extraction and complete hash verification", async () =>
                {
                    using Stream manifest = File.OpenRead(Path.Combine(args[4], "payload.json"));
                    var real = new BundlePackage(JsonFiles.Read<PayloadManifest>(manifest), capabilities, () => File.OpenRead(Path.Combine(args[4], "payload.zip")));
                    IsoFixture fixture = Fixtures.Iso(NewPath("actual-payload.iso"));
                    BuildResult result = await new BuildPipeline(real).BuildAsync(Request(fixture.Path, NewPath("actual-payload-output")), null, null, CancellationToken.None);
                    VerifyRecordedHashes(result.OutputDirectory);
                    string builtToml = File.ReadAllText(Path.Combine(result.OutputDirectory, "userdata", "simpsons.toml"));
                    Check(builtToml.Split('\n', StringSplitOptions.TrimEntries).Contains("user_language = 1"),
                        "The generated PS5 folder must start in English by default.");
                });
            }
        }
        catch (Exception e) { Console.Error.WriteLine(e); exitCode = 1; }
        if (BuildPipeline.HashFile(args[0], CancellationToken.None) != Sha(capabilityBytes))
        {
            exitCode = 1;
            Results.Add(new { name = "capability input remained unchanged during the run", status = "FAIL", error = "capabilities.json changed while the tests were running; rerun with the final manifest." });
        }
        string sourceRoot = Path.GetFullPath(Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location)!);
        while (!File.Exists(Path.Combine(sourceRoot, "Directory.Build.props")))
            sourceRoot = Path.GetDirectoryName(sourceRoot) ?? throw new Exception("Builder source root could not be located for provenance.");
        var sources = Directory.EnumerateFiles(sourceRoot, "*", SearchOption.AllDirectories)
            .Where(p => !Path.GetRelativePath(sourceRoot, p).Split(Path.DirectorySeparatorChar).Any(c => c is "bin" or "obj" or "publish"))
            .Where(p => Path.GetExtension(p) is ".cs" or ".csproj" or ".props" or ".manifest" or ".py")
            .OrderBy(p => p, StringComparer.Ordinal).ToDictionary(p => Path.GetRelativePath(sourceRoot, p).Replace('\\', '/'), p => BuildPipeline.HashFile(p, CancellationToken.None));
        var report = new
        {
            status = exitCode == 0 ? "PASS" : "FAIL", scope = "Synthetic XDVDFS/XEX fixtures; core runs on the reported host. No real game ISO and no PS5 execution.",
            host = new { os = Environment.OSVersion.ToString(), framework = System.Runtime.InteropServices.RuntimeInformation.FrameworkDescription },
            workDirectory = work, extractXiso = args.Length >= 4 ? Path.GetFullPath(args[3]) : null,
            capabilitySha256 = Sha(capabilityBytes), tests = Results,
            sourceSha256 = sources, upstreamComparisons = Comparisons,
            upstreamDifferential = args.Length >= 4 ? "RUN" : "NOT_RUN", windowsGui = "NOT_RUN", hardware = "NOT_RUN"
        };
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(args[2]))!);
        File.WriteAllText(args[2], JsonSerializer.Serialize(report, JsonFiles.Options) + "\n");
        Console.WriteLine($"{(exitCode == 0 ? "PASS" : "FAIL")}: {Results.Count} scenarios; report {args[2]}");
        return exitCode;
    }

    private static async Task Run(string name, Func<Task> test)
    {
        var timer = Stopwatch.StartNew();
        try { await test(); Results.Add(new { name, status = "PASS", seconds = timer.Elapsed.TotalSeconds }); Console.WriteLine("PASS " + name); }
        catch (Exception e) { Results.Add(new { name, status = "FAIL", error = e.ToString(), seconds = timer.Elapsed.TotalSeconds }); throw; }
    }
    private static void Reject(Action action)
    {
        try { action(); } catch (BuilderException) { return; }
        throw new Exception("Malformed input was accepted.");
    }
    private static async Task RejectAsync(Func<Task> action)
    {
        try { await action(); } catch (BuilderException) { return; }
        throw new Exception("Invalid build was accepted.");
    }
    private static void RejectSource(string path) => Reject(() => GameSource.Open(new(InputKind.Iso, path), "45410809", CancellationToken.None).Dispose());
    private static void VerifyGameBytes(string root, Dictionary<string, byte[]> expected, bool canonical = true)
    {
        foreach ((string relative, byte[] bytes) in expected)
        {
            string path = Path.Combine(root, (canonical ? SafePaths.CanonicalGamePath(relative) : relative).Replace('/', Path.DirectorySeparatorChar));
            Check(File.Exists(path) && File.ReadAllBytes(path).SequenceEqual(bytes), "Game bytes differ: " + relative);
        }
    }
    private static void VerifyRecordedHashes(string root)
    {
        using JsonDocument info = JsonDocument.Parse(File.ReadAllText(Path.Combine(root, "build-info.json")));
        foreach (JsonProperty file in info.RootElement.GetProperty("files").EnumerateObject())
            Check(BuildPipeline.HashFile(SafePaths.Under(root, file.Name), CancellationToken.None) == file.Value.GetString(), "Final manifest hash mismatch: " + file.Name);
        int produced = Directory.EnumerateFiles(Path.Combine(root, "gamedata"), "*", SearchOption.AllDirectories).Count();
        int recorded = info.RootElement.GetProperty("files").EnumerateObject().Count(f => f.Name.StartsWith("gamedata/"));
        Check(produced == recorded, "Game file list does not reflect patches.");
    }
    private static void CompareUpstream(string executable, string iso, string output)
    {
        string directory = NewPath("extract-xiso");
        var start = new ProcessStartInfo(Path.GetFullPath(executable)) { RedirectStandardError = true, RedirectStandardOutput = true, UseShellExecute = false };
        start.ArgumentList.Add("-x"); start.ArgumentList.Add("-d"); start.ArgumentList.Add(directory); start.ArgumentList.Add(iso);
        using Process process = Process.Start(start) ?? throw new Exception("Unable to start extract-xiso.");
        Task<string> stdout = process.StandardOutput.ReadToEndAsync(); Task<string> stderr = process.StandardError.ReadToEndAsync();
        if (!process.WaitForExit(30_000)) { process.Kill(true); throw new Exception("extract-xiso timed out."); }
        Task.WaitAll(stdout, stderr);
        File.WriteAllText(directory + ".log", stdout.Result + stderr.Result);
        Check(process.ExitCode == 0, "extract-xiso failed: " + stderr.Result);
        var upstream = Directory.EnumerateFiles(directory, "*", SearchOption.AllDirectories).ToDictionary(p => SafePaths.CanonicalGamePath(Path.GetRelativePath(directory, p).Replace('\\', '/')), p => BuildPipeline.HashFile(p, CancellationToken.None), StringComparer.Ordinal);
        var builder = Directory.EnumerateFiles(output, "*", SearchOption.AllDirectories).ToDictionary(p => Path.GetRelativePath(output, p).Replace('\\', '/'), p => BuildPipeline.HashFile(p, CancellationToken.None), StringComparer.Ordinal);
        Check(upstream.Count == builder.Count && upstream.All(kv => builder.TryGetValue(kv.Key, out string? value) && value == kv.Value), "Builder extraction differs from upstream extract-xiso.");
        Comparisons.Add(new { fixture = Path.GetFileName(iso), files = upstream.Count, bytesEqual = true,
            extractorSha256 = BuildPipeline.HashFile(executable, CancellationToken.None),
            fileListSha256 = Sha(JsonSerializer.SerializeToUtf8Bytes(upstream.OrderBy(p => p.Key, StringComparer.Ordinal))),
            logSha256 = Sha(Encoding.UTF8.GetBytes(stdout.Result + stderr.Result)), exitCode = process.ExitCode });
    }
    private sealed class InlineProgress(Action<BuildProgress> action) : IProgress<BuildProgress> { public void Report(BuildProgress value) => action(value); }
}
