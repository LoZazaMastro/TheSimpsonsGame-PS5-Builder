using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace Simpsons.Builder.Tests;

/// <summary>
/// Regressions for the erroneous 45410806 whitelist. The expected Simpsons ID
/// stays a literal in these tests, independent of the production constant.
/// Metadata combinations below are synthetic format tests, not a retail SKU
/// inventory or evidence that the PS5 game engine ran a regional executable.
/// </summary>
internal static class DiscIdentityChecks
{
    private sealed record Variant(string Name, uint MediaId, uint Version, uint BaseVersion,
        uint RegionMask, string[] Languages);

    private static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    private static string Hash(byte[] bytes) => Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant();

    internal static async Task RunAllAsync(Func<string, Func<Task>, Task> run, BundlePackage bundle,
        Capabilities capabilities, Func<string, string> newPath)
    {
        await run("Simpsons identity is 45410809; obsolete 45410806 payload metadata is rejected", () =>
        {
            Check(BinaryPrimitives.ReadUInt32BigEndian(Fixtures.Xex().AsSpan(76)) == 0x45410809,
                "The synthetic default must identify The Simpsons Game, not repeat an obsolete builder ID.");
            Check(bundle.Manifest.XboxTitleId == "45410809", "The payload fixture contains the wrong Xbox title ID.");
            bundle.Validate();
            BundlePackage obsolete = Fixtures.Bundle(capabilities).Bundle;
            obsolete.Manifest.XboxTitleId = "45410806";
            ExpectRejection(obsolete.Validate);
            return Task.CompletedTask;
        });

        Variant[] variants =
        [
            new("PAL mask and Italian/Spanish files", 0x11223344, 0x00000100, 0, 0x00FF0000, ["italian", "spanish"]),
            new("NTSC-U mask and English files", 0x10203040, 0x00010000, 0x00000100, 0x000000FF, ["english"]),
            new("PAL mask and French/German files", 0xAABBCCDD, 0x00020000, 0x00010000, 0x00FF0000, ["french", "german"]),
            new("NTSC-J mask and alternate language files", 0x55667788, 0x00020100, 0x00020000, 0x0000FF00, ["japanese"]),
            new("all-region mask and mixed language files", 0xF1234567, 0x00030000, 0x00020100, 0xFFFFFFFF, ["english", "italian", "spanish", "french", "german"])
        ];
        foreach (Variant variant in variants)
        {
            foreach (InputKind kind in new[] { InputKind.Iso, InputKind.GameDataFolder })
            {
                await run($"45410809 {kind} build accepts varied media/version/region/language: {variant.Name}", async () =>
                {
                    Dictionary<string, byte[]> files = VariantFiles(variant);
                    string input = CreateInput(kind, files, newPath);
                    string? isoHash = kind == InputKind.Iso ? BuildPipeline.HashFile(input, CancellationToken.None) : null;
                    var messages = new List<string>();
                    using (GameSource source = GameSource.Open(new(kind, input), "45410809", CancellationToken.None, log: messages.Add))
                    {
                        Check(source.Identity.TitleId == "45410809", "The correct title ID was not read from the XEX execution-info header.");
                        Check(source.Identity.MediaId == variant.MediaId.ToString("X8"), "The disc media ID was not preserved.");
                        Check(source.Identity.Version == variant.Version.ToString("X8"), "The disc version was not preserved.");
                        Check(source.Identity.DiscNumber == 1 && source.Identity.DiscCount == 1, "The disc identity was altered.");
                    }
                    CheckDecision(messages, "45410809", accepted: true);
                    var request = new BuildRequest(new(kind, input), newPath("identity-output"), capabilities.Defaults(losslessAvailable: false));
                    BuildResult result = await new BuildPipeline(bundle).BuildAsync(request, null, null, CancellationToken.None);
                    Check(result.XboxTitleId == "45410809" && result.MediaId == variant.MediaId.ToString("X8"),
                        "The completed build does not retain the source disc identity.");
                    Check(result.DefaultXexSha256 == Hash(files["DEFAULT.XEX"]), "The build did not preserve the source XEX bytes.");
                    Check(Path.GetFileName(result.OutputDirectory) == "PPSA74207", "The PS5 title output directory changed.");
                    foreach ((string relative, byte[] bytes) in files)
                    {
                        string outputFile = SafePaths.Under(Path.Combine(result.OutputDirectory, "gamedata"), SafePaths.CanonicalGamePath(relative));
                        Check(File.ReadAllBytes(outputFile).SequenceEqual(bytes), "Regional source data was changed or omitted: " + relative);
                        if (kind == InputKind.GameDataFolder)
                            Check(File.ReadAllBytes(SafePaths.Under(input, relative)).SequenceEqual(bytes), "The input game folder was modified.");
                    }
                    if (isoHash is not null)
                        Check(BuildPipeline.HashFile(input, CancellationToken.None) == isoHash, "The input ISO was modified.");
                });
            }
        }

        foreach (InputKind kind in new[] { InputKind.Iso, InputKind.GameDataFolder })
        {
            await run($"obsolete configured title ID is a builder error, not a wrong-game claim: {kind}", () =>
            {
                string input = CreateInput(kind, Fixtures.GameFiles(), newPath);
                BuilderException failure = ExpectRejection(() =>
                    GameSource.Open(new(kind, input), "45410806", CancellationToken.None).Dispose());
                Check(failure.Message.Contains("builder", StringComparison.OrdinalIgnoreCase)
                    && !failure.Message.Contains("different game", StringComparison.OrdinalIgnoreCase),
                    "A valid Simpsons source was blamed for an invalid builder configuration.");
                return Task.CompletedTask;
            });
            foreach (uint wrongTitle in new uint[] { 0x45410806, 0x12345678 })
            {
                await run($"unrelated title {wrongTitle:X8} remains rejected in {kind} with identity diagnostics", async () =>
                {
                    Dictionary<string, byte[]> files = Fixtures.GameFiles();
                    files["DEFAULT.XEX"] = Fixtures.Xex(wrongTitle);
                    string input = CreateInput(kind, files, newPath);
                    var messages = new List<string>();
                    BuilderException failure = ExpectRejection(() =>
                        GameSource.Open(new(kind, input), "45410809", CancellationToken.None, log: messages.Add).Dispose());
                    Check(failure.Message.Contains(wrongTitle.ToString("X8"), StringComparison.OrdinalIgnoreCase),
                        "The error does not identify the observed unrelated game.");
                    CheckDecision(messages, wrongTitle.ToString("X8"), accepted: false);
                    string output = newPath("wrong-title-output");
                    var request = new BuildRequest(new(kind, input), output, capabilities.Defaults(losslessAvailable: false));
                    try
                    {
                        await new BuildPipeline(bundle).BuildAsync(request, null, null, CancellationToken.None);
                        throw new Exception("An unrelated game was packaged as The Simpsons Game.");
                    }
                    catch (BuilderException) { }
                    Check(!Directory.Exists(Path.Combine(output, "PPSA74207")), "An unrelated title was published.");
                    Check(!Directory.Exists(output) || !Directory.EnumerateDirectories(output, ".*.building-*").Any(),
                        "Wrong-game rejection left a staging directory behind.");
                });
            }
        }
    }

    private static Dictionary<string, byte[]> VariantFiles(Variant variant)
    {
        Dictionary<string, byte[]> files = Fixtures.GameFiles();
        files["DEFAULT.XEX"] = Fixtures.Xex(title: 0x45410809, mediaId: variant.MediaId,
            version: variant.Version, baseVersion: variant.BaseVersion, region: variant.RegionMask);
        foreach (string path in files.Keys.Where(p => p.StartsWith("MOVIES/", StringComparison.Ordinal)).ToList()) files.Remove(path);
        foreach (string language in variant.Languages)
            files.Add($"MOVIES/{language}/intro.vp6", Encoding.UTF8.GetBytes($"SYNTHETIC TEST MOVIE BYTES ({language})"));
        return files;
    }

    private static string CreateInput(InputKind kind, Dictionary<string, byte[]> files, Func<string, string> newPath) =>
        kind == InputKind.Iso ? Fixtures.Iso(newPath("identity.iso"), files: files).Path
            : Fixtures.Folder(newPath("identity-gamedata"), files);

    private static BuilderException ExpectRejection(Action action)
    {
        try { action(); }
        catch (BuilderException error) { return error; }
        throw new Exception("Invalid identity was accepted.");
    }

    private static void CheckDecision(IEnumerable<string> messages, string actualTitleId, bool accepted)
    {
        const string prefix = "source.identity.decision ";
        string? message = messages.SingleOrDefault(m => m.StartsWith(prefix, StringComparison.Ordinal));
        Check(message is not null, "The source identity decision was not logged.");
        using JsonDocument data = JsonDocument.Parse(message![prefix.Length..]);
        JsonElement decision = data.RootElement;
        Check(decision.GetProperty("expectedTitleId").GetString() == "45410809"
            && decision.GetProperty("actualTitleId").GetString() == actualTitleId
            && decision.GetProperty("accepted").GetBoolean() == accepted,
            "The identity decision log does not contain the actual title, expected title and outcome.");
    }
}
