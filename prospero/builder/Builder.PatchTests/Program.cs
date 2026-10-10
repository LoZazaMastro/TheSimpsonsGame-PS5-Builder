using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using Simpsons.Builder;

string root = Path.Combine(Path.GetTempPath(), "simpsons-patch-tests-" + Guid.NewGuid().ToString("N"));
Directory.CreateDirectory(root);
var results = new List<object>();
int passed = 0, failed = 0;
string Fixture(string name)
{
    string dir = Path.Combine(root, name);
    Directory.CreateDirectory(dir);
    return dir;
}
void Check(bool condition, string detail)
{
    if (!condition) throw new InvalidOperationException(detail);
}
void Test(string name, Action action)
{
    try { action(); passed++; results.Add(new { name, status = "PASS" }); }
    catch (Exception e) { failed++; results.Add(new { name, status = "FAIL", error = e.ToString() }); }
}
Dictionary<string, string> Snapshot(string dir) => Directory.EnumerateFiles(dir, "*", SearchOption.AllDirectories)
    .ToDictionary(p => Path.GetRelativePath(dir, p), p => Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(p))), StringComparer.Ordinal);
void Same(Dictionary<string, string> before, string dir)
{
    Dictionary<string, string> after = Snapshot(dir);
    Check(before.Count == after.Count && before.All(p => after.TryGetValue(p.Key, out string? hash) && hash == p.Value), "Unexpected filesystem change");
}
void RejectUnchanged(string dir, GamePatchOptions options)
{
    var before = Snapshot(dir);
    bool rejected = false;
    try { GamePatches.Apply(dir, options, CancellationToken.None); }
    catch (BuilderException) { rejected = true; }
    Check(rejected, "Invalid patch was accepted");
    Same(before, dir);
}
string Lua(string nl = "\n") => string.Join(nl,
    "-- Synthetic test data; byte \u0080 is preserved, not interpreted.",
    "episode = NewEpisode(game, \"LAND_OF_CHOCOLATE\")",
    "  episode:SetDefault() -- initial start",
    "episode:SetValue(1)",
    "\tepisode = NewEpisode(game, \"BARTMAN_BEGINS\")",
    "episode:SetValue(2)",
    "episode = NewEpisode(game, \"TREEHUGGER\")",
    "episode:SetValue(3)", "");
void WriteLua(string dir, string text) => File.WriteAllBytes(Path.Combine(dir, "simpsons_gameflow.lua"), Encoding.Latin1.GetBytes(text));
string ReadLua(string dir) => Encoding.Latin1.GetString(File.ReadAllBytes(Path.Combine(dir, "simpsons_gameflow.lua")));
void Logo(string dir, string language, string name)
{
    string target = Path.Combine(dir, "movies", language, name);
    Directory.CreateDirectory(Path.GetDirectoryName(target)!);
    File.WriteAllBytes(target, Encoding.ASCII.GetBytes("Synthetic logo placeholder: " + name));
}

try
{
    Test("default options preserve existing custom game data", () =>
    {
        string dir = Fixture("default"); WriteLua(dir, Lua());
        File.WriteAllText(Path.Combine(dir, "simpsons_gameflow.lua.original"), "external original");
        Logo(dir, "it", "ealogo.vp6.disabled");
        var before = Snapshot(dir);
        GamePatchResult result = GamePatches.Apply(dir, new(), CancellationToken.None);
        Check(result.ChangedRelativePaths.Count == 0 && result.DeletedRelativePaths.Count == 0, "Default patch reports changes");
        Same(before, dir);
    });
    Test("all six intro variants and case-insensitive languages preserve bytes", () =>
    {
        string dir = Fixture("logos");
        string[] logos = ["ealogo", "ealogo_sd", "foxlogo", "foxlogo_sd", "gracielogo", "gracielogo_sd"];
        foreach (string language in new[] { "it", "ES" })
            foreach (string logo in logos) Logo(dir, language, logo.ToUpperInvariant() + ".VP6");
        Logo(dir, "it", "opening_scene.vp6");
        var before = Snapshot(dir);
        GamePatchResult result = GamePatches.Apply(dir, new(true), CancellationToken.None);
        Check(result.ChangedRelativePaths.Count == 12 && result.DeletedRelativePaths.Count == 12, "Wrong set of logo moves");
        foreach (string deleted in result.DeletedRelativePaths)
        {
            Check(!File.Exists(Path.Combine(dir, deleted)), "Active logo still exists");
            Check(Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(Path.Combine(dir, deleted + ".disabled")))) == before[deleted], "Logo bytes changed");
        }
        Check(File.Exists(Path.Combine(dir, "movies", "it", "opening_scene.vp6")), "Non-logo movie was changed");
        var second = GamePatches.Apply(dir, new(true), CancellationToken.None);
        Check(second.ChangedRelativePaths.Count == 0, "Logo patch is not idempotent");
    });
    Test("a duplicate disabled logo rejects the whole requested transaction", () =>
    {
        string dir = Fixture("logo-conflict"); WriteLua(dir, Lua());
        Logo(dir, "it", "ealogo.vp6"); Logo(dir, "it", "ealogo.vp6.disabled");
        RejectUnchanged(dir, new(true, "BARTMAN_BEGINS"));
    });
    foreach (string nl in new[] { "\n", "\r\n" })
    {
        string newlineLabel = nl.Length == 1 ? "LF" : "CRLF";
        Test("episode selection, switch and original restore are byte-exact: " + newlineLabel, () =>
        {
            string dir = Fixture("episode-" + newlineLabel); string original = Lua(nl); WriteLua(dir, original);
            var first = GamePatches.Apply(dir, new(false, "BARTMAN_BEGINS"), CancellationToken.None);
            Check(first.ChangedRelativePaths.Count == 2 && first.DeletedRelativePaths.Count == 0, "Original backup was not recorded");
            string expected = original.Replace("  episode:SetDefault() -- initial start",
                "  -- episode:SetDefault() -- initial start  -- start episode moved by the launcher", StringComparison.Ordinal)
                .Replace("\tepisode = NewEpisode(game, \"BARTMAN_BEGINS\")" + nl,
                    "\tepisode = NewEpisode(game, \"BARTMAN_BEGINS\")" + nl +
                    "\tepisode:SetDefault()  -- start episode chosen in the launcher" + nl, StringComparison.Ordinal);
            Check(ReadLua(dir) == expected, "Patch differs from upstream expected transformation");
            Check(Encoding.Latin1.GetString(File.ReadAllBytes(Path.Combine(dir, "simpsons_gameflow.lua.original"))) == original, "Backup changed original bytes");
            var repeat = GamePatches.Apply(dir, new(false, "BARTMAN_BEGINS"), CancellationToken.None);
            Check(repeat.ChangedRelativePaths.Count == 0, "Repeated episode selection rewrote the gameflow");
            GamePatches.Apply(dir, new(false, "TREEHUGGER"), CancellationToken.None);
            var restored = GamePatches.Apply(dir, new(false, "LAND_OF_CHOCOLATE"), CancellationToken.None);
            Check(ReadLua(dir) == original, "Restored gameflow is not byte-exact");
            Check(!File.Exists(Path.Combine(dir, "simpsons_gameflow.lua.original")) && restored.DeletedRelativePaths.Count == 1, "Restored backup not retired");
        });
    }
    Test("selecting the original start does not create a backup", () =>
    {
        string dir = Fixture("original"); WriteLua(dir, Lua()); var before = Snapshot(dir);
        var result = GamePatches.Apply(dir, new(false, "LAND_OF_CHOCOLATE"), CancellationToken.None);
        Check(result.ChangedRelativePaths.Count == 0, "Unnecessary original rewrite"); Same(before, dir);
    });
    Test("episode at end of file without final newline", () =>
    {
        string dir = Fixture("no-newline"); WriteLua(dir,
            "episode = NewEpisode(game, \"LAND_OF_CHOCOLATE\")\nepisode:SetDefault()\nepisode = NewEpisode(game, \"TREEHUGGER\")");
        GamePatches.Apply(dir, new(false, "TREEHUGGER"), CancellationToken.None);
        Check(ReadLua(dir).EndsWith("episode:SetDefault()  -- start episode chosen in the launcher\n", StringComparison.Ordinal), "Missing last default call");
    });
    Test("external edits with a backup are rejected without touching files", () =>
    {
        string dir = Fixture("external-edit"); WriteLua(dir, Lua());
        GamePatches.Apply(dir, new(false, "BARTMAN_BEGINS"), CancellationToken.None);
        File.AppendAllText(Path.Combine(dir, "simpsons_gameflow.lua"), "-- external change\n");
        RejectUnchanged(dir, new(false, "TREEHUGGER"));
    });
    var malformed = new Dictionary<string, string>
    {
        ["unknown-episode"] = Lua(),
        ["duplicate-episode"] = Lua().Replace("TREEHUGGER", "BARTMAN_BEGINS", StringComparison.Ordinal),
        ["multiple-defaults"] = Lua() + "episode:SetDefault()\n",
        ["missing-default"] = Lua().Replace("episode:SetDefault()", "-- no default", StringComparison.Ordinal),
        ["default-before-episodes"] = "episode:SetDefault()\n" + Lua().Replace("episode:SetDefault()", "-- default removed", StringComparison.Ordinal),
        ["no-episodes"] = "-- synthetic data without episodes\n"
    };
    foreach ((string name, string text) in malformed)
        Test("reject malformed or unavailable selection: " + name, () =>
        {
            string dir = Fixture(name); WriteLua(dir, text); Logo(dir, "it", "foxlogo.vp6");
            RejectUnchanged(dir, new(true, name == "unknown-episode" ? "UNKNOWN_EPISODE" : "TREEHUGGER"));
        });
    Test("cancellation before patch leaves staging unchanged", () =>
    {
        string dir = Fixture("cancel"); WriteLua(dir, Lua()); var before = Snapshot(dir);
        using var cancellation = new CancellationTokenSource(); cancellation.Cancel();
        bool cancelled = false;
        try { GamePatches.Apply(dir, new(false, "BARTMAN_BEGINS"), cancellation.Token); }
        catch (OperationCanceledException) { cancelled = true; }
        Check(cancelled, "Cancellation ignored"); Same(before, dir);
    });
    Test("episode choices are read from the actual source rather than a fixed list", () =>
    {
        string dir = Fixture("source-choices"); WriteLua(dir, Lua()); Logo(dir, "it", "ealogo.vp6");
        var xex = new byte[256]; "XEX2"u8.CopyTo(xex);
        BinaryPrimitives.WriteUInt32BigEndian(xex.AsSpan(8), 128);
        BinaryPrimitives.WriteUInt32BigEndian(xex.AsSpan(20), 1);
        BinaryPrimitives.WriteUInt32BigEndian(xex.AsSpan(24), 0x00040006);
        BinaryPrimitives.WriteUInt32BigEndian(xex.AsSpan(28), 64);
        BinaryPrimitives.WriteUInt32BigEndian(xex.AsSpan(76), 0x45410809);
        File.WriteAllBytes(Path.Combine(dir, "default.xex"), xex);
        using var source = GameSource.Open(new(InputKind.GameDataFolder, dir), "45410809", CancellationToken.None);
        var choices = GamePatches.ReadEpisodes(source, CancellationToken.None);
        Check(choices.Select(c => c.Id).SequenceEqual(new[] { "LAND_OF_CHOCOLATE", "BARTMAN_BEGINS", "TREEHUGGER" }), "Choices do not match this disc");
        Check(choices[1].Label == "Bartman Begins", "Episode label is not English");
    });
}
finally { Directory.Delete(root, recursive: true); }
var report = new { schemaVersion = 1, status = failed == 0 ? "PASS" : "FAIL", passed, failed,
    scope = "Synthetic local gameflow and logo fixtures; no real game data or console execution.", results };
string json = JsonSerializer.Serialize(report, new JsonSerializerOptions { WriteIndented = true });
if (args.Length > 0) File.WriteAllText(args[0], json + "\n");
Console.WriteLine(json);
return failed == 0 ? 0 : 1;
