using System.Globalization;
using System.Text;
using System.Text.RegularExpressions;

namespace Simpsons.Builder;

public sealed record EpisodeChoice(string Id, string Label);
public sealed record GamePatchOptions(bool SkipIntroLogos = false, string? StartEpisode = null);
public sealed record GamePatchResult(IReadOnlyList<string> ChangedRelativePaths,
    IReadOnlyList<string> DeletedRelativePaths);

/// <summary>
/// Optional, explicit changes to the builder's unpublished staging directory.
/// The ISO and source folder are never modified. Behaviour follows the upstream
/// launcher's intro-logo and start-episode patches, without executing game Lua.
/// Latin-1 is used as a reversible byte mapping, so UTF-8, legacy bytes, BOMs and
/// line endings survive unchanged outside the ASCII statements being edited.
/// The build transaction discards the entire staging directory on any failure.
/// </summary>
public static class GamePatches
{
    private const string GameflowName = "simpsons_gameflow.lua";
    private const int MaximumLuaBytes = 16 * 1024 * 1024;
    private static readonly Regex EpisodePattern = new(
        "^([ \\t]*)episode\\s*=\\s*NewEpisode\\(\\s*game\\s*,\\s*\"([A-Za-z0-9_]+)\"",
        RegexOptions.Multiline | RegexOptions.CultureInvariant, TimeSpan.FromSeconds(2));
    private static readonly Regex DefaultPattern = new(
        @"^[ \t]*episode:SetDefault\(\)[^\r\n]*",
        RegexOptions.Multiline | RegexOptions.CultureInvariant, TimeSpan.FromSeconds(2));
    private static readonly HashSet<string> LogoNames = new(StringComparer.OrdinalIgnoreCase)
    {
        "ealogo.vp6", "ealogo_sd.vp6", "foxlogo.vp6", "foxlogo_sd.vp6",
        "gracielogo.vp6", "gracielogo_sd.vp6"
    };
    private static readonly IReadOnlyDictionary<string, string> EpisodeNames =
        new Dictionary<string, string>(StringComparer.Ordinal)
        {
            ["SPR_HUB"] = "Springfield (hub)",
            ["LAND_OF_CHOCOLATE"] = "Land of Chocolate (original start)",
            ["BARTMAN_BEGINS"] = "Bartman Begins",
            ["EIGHTY_BITES"] = "Around the World in 80 Bites",
            ["TREEHUGGER"] = "Lisa the Tree Hugger",
            ["MOB_RULES"] = "Mob Rules",
            ["CHEATER"] = "Enter the Cheatrix",
            ["DOLPHINS"] = "Day of the Dolphins",
            ["COLOSSAL_DONUT"] = "The Colossal Donut",
            ["SPRINGFIELD_STOOD_STILL"] = "The Day the Earth Stood Stupid",
            ["BARGAIN_BIN"] = "Bargain Bin",
            ["GAME_HUB"] = "Video game world (hub)",
            ["NEVERQUEST"] = "NeverQuest",
            ["GRAND_THEFT_SCRATCHY"] = "Grand Theft Scratchy",
            ["MEDAL_OF_HOMER"] = "Medal of Homer",
            ["BIG_SUPER"] = "Big Super Happy Fun Fun Game",
            ["RHYMES_WITH_COMPLAINING"] = "Rhymes with Complaining",
            ["MEET_THY_PLAYER"] = "Meet Thy Player"
        };

    public static IReadOnlyList<EpisodeChoice> ReadEpisodes(GameSource source, CancellationToken ct)
    {
        ct.ThrowIfCancellationRequested();
        GameEntry? entry = source.Entries.SingleOrDefault(e => !e.IsDirectory &&
            e.RelativePath.Equals(GameflowName, StringComparison.OrdinalIgnoreCase));
        if (entry is null) return [];
        GameEntry? backup = source.Entries.SingleOrDefault(e => !e.IsDirectory &&
            e.RelativePath.Equals(GameflowName + ".original", StringComparison.OrdinalIgnoreCase));
        entry = backup ?? entry;
        if (entry.Length > MaximumLuaBytes)
            throw new BuilderException("The episode list is too large or damaged. Keep the original starting episode.");
        using Stream stream = source.OpenFile(entry);
        var bytes = new byte[checked((int)entry.Length)];
        stream.ReadExactly(bytes);
        ct.ThrowIfCancellationRequested();
        string text = Decode(bytes);
        List<EpisodeBlock> blocks = ValidateGameflow(text);
        return blocks.Select(b => new EpisodeChoice(b.Id, EpisodeNames.TryGetValue(b.Id, out string? name)
            ? name : CultureInfo.InvariantCulture.TextInfo.ToTitleCase(b.Id.Replace('_', ' ').ToLowerInvariant()))).ToList();
    }

    public static GamePatchResult Apply(string stagedGameData, GamePatchOptions options,
        CancellationToken ct, Action<string>? log = null)
    {
        ct.ThrowIfCancellationRequested();
        if (!options.SkipIntroLogos && string.IsNullOrEmpty(options.StartEpisode)) return new([], []);
        string root = Path.GetFullPath(stagedGameData);
        SafePaths.NoReparsePoints(root);
        if (!Directory.Exists(root)) throw new BuilderException("The prepared game data folder is missing.");
        var changed = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var deleted = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var moves = new List<(string From, string To)>();
        string? lua = null, backup = null;
        byte[]? originalBytes = null, patchedBytes = null;
        bool createBackup = false, deleteBackup = false;

        // Resolve and validate every requested change before writing anything.
        if (options.SkipIntroLogos)
        {
            string? movies = FindChild(root, "movies", directory: true);
            if (movies is not null)
            {
                foreach (string language in Directory.EnumerateDirectories(movies).Order(StringComparer.Ordinal))
                {
                    ct.ThrowIfCancellationRequested();
                    SafePaths.NoReparsePoints(language);
                    foreach (string file in Directory.EnumerateFiles(language).Order(StringComparer.Ordinal))
                    {
                        if (!LogoNames.Contains(Path.GetFileName(file))) continue;
                        SafePaths.NoReparsePoints(file);
                        string name = Path.GetFileName(file) + ".disabled";
                        if (FindChild(language, name, directory: null) is not null)
                            throw new BuilderException("An intro video has both active and disabled copies: " +
                                Relative(root, file) + ". Keep the existing game data or resolve the duplicate first.");
                        moves.Add((file, Path.Combine(language, name)));
                    }
                }
            }
        }

        if (!string.IsNullOrEmpty(options.StartEpisode))
        {
            lua = FindChild(root, GameflowName, directory: false)
                ?? throw new BuilderException("This game copy does not contain an editable episode list. Keep the original starting episode.");
            backup = FindChild(root, GameflowName + ".original", directory: false);
            byte[] currentBytes = ReadLua(lua, ct);
            originalBytes = backup is null ? currentBytes : ReadLua(backup, ct);
            string original = Decode(originalBytes);
            List<EpisodeBlock> blocks = ValidateGameflow(original);
            if (!blocks.Any(b => b.Id == options.StartEpisode))
                throw new BuilderException("The selected episode is not present in this game copy. Choose an episode from its list.");
            string originalStart = StartingEpisode(original, blocks)!;
            if (backup is not null)
            {
                string current = Decode(currentBytes);
                List<EpisodeBlock> currentBlocks = ValidateGameflow(current);
                string selected = StartingEpisode(current, currentBlocks)!;
                if (!blocks.Any(b => b.Id == selected) || current != MoveDefault(original, selected))
                    throw new BuilderException("The episode list was changed outside the launcher. The original backup is preserved; keep the current starting episode.");
            }
            patchedBytes = Encoding.Latin1.GetBytes(MoveDefault(original, options.StartEpisode));
            createBackup = backup is null && options.StartEpisode != originalStart;
            deleteBackup = backup is not null && options.StartEpisode == originalStart;
            backup ??= lua + ".original";
        }

        ct.ThrowIfCancellationRequested();
        if (lua is not null && patchedBytes is not null && originalBytes is not null && backup is not null)
        {
            if (createBackup)
            {
                WriteNew(backup, originalBytes, ct);
                changed.Add(Relative(root, backup));
            }
            if (!ReadLua(lua, ct).AsSpan().SequenceEqual(patchedBytes))
            {
                WriteAtomic(lua, patchedBytes, ct);
                changed.Add(Relative(root, lua));
            }
            if (deleteBackup)
            {
                ct.ThrowIfCancellationRequested();
                File.Delete(backup);
                deleted.Add(Relative(root, backup));
            }
            log?.Invoke("Starting episode prepared: " + options.StartEpisode + "; affects new games only.");
        }
        foreach ((string from, string to) in moves)
        {
            ct.ThrowIfCancellationRequested();
            File.Move(from, to, overwrite: false);
            deleted.Add(Relative(root, from));
            changed.Add(Relative(root, to));
        }
        if (options.SkipIntroLogos)
            log?.Invoke(moves.Count == 0 ? "No active intro logo videos to skip; game files were kept." :
                "Intro logo videos disabled in prepared output: " + moves.Count + ". Their original bytes are preserved.");
        return new(changed.Order(StringComparer.Ordinal).ToArray(), deleted.Order(StringComparer.Ordinal).ToArray());
    }

    private sealed record EpisodeBlock(string Id, int Start, int End, string Indent);

    private static List<EpisodeBlock> Episodes(string text)
    {
        MatchCollection matches = EpisodePattern.Matches(text);
        return matches.Select((m, index) => new EpisodeBlock(m.Groups[2].Value, m.Index,
            index + 1 < matches.Count ? matches[index + 1].Index : text.Length, m.Groups[1].Value)).ToList();
    }

    private static List<EpisodeBlock> ValidateGameflow(string text)
    {
        List<EpisodeBlock> blocks = Episodes(text);
        if (blocks.Count == 0 || blocks.Select(b => b.Id).Distinct(StringComparer.Ordinal).Count() != blocks.Count ||
            DefaultPattern.Matches(text).Count != 1 || StartingEpisode(text, blocks) is null)
            throw new BuilderException("The episode list has an unexpected format. Keep the original starting episode; no episode data has been changed.");
        return blocks;
    }

    private static string? StartingEpisode(string text, List<EpisodeBlock> blocks)
    {
        Match call = DefaultPattern.Match(text);
        return !call.Success ? null : blocks.FirstOrDefault(b => b.Start <= call.Index && call.Index < b.End)?.Id;
    }

    private static string MoveDefault(string original, string episodeId)
    {
        List<EpisodeBlock> blocks = ValidateGameflow(original);
        if (episodeId == StartingEpisode(original, blocks)) return original;
        if (!blocks.Any(b => b.Id == episodeId)) throw new BuilderException("The requested starting episode is unavailable.");
        string newline = original.Contains("\r\n", StringComparison.Ordinal) ? "\r\n" : "\n";
        string text = DefaultPattern.Replace(original, m =>
            m.Value.Replace("episode:SetDefault()", "-- episode:SetDefault()", StringComparison.Ordinal) +
            "  -- start episode moved by the launcher", count: 1);
        Match target = EpisodePattern.Matches(text).Single(m => m.Groups[2].Value == episodeId);
        int lineEnd = text.IndexOf('\n', target.Index + target.Length);
        if (lineEnd < 0) { text += newline; lineEnd = text.Length; }
        else lineEnd++;
        text = text.Insert(lineEnd, target.Groups[1].Value +
            "episode:SetDefault()  -- start episode chosen in the launcher" + newline);
        List<EpisodeBlock> result = ValidateGameflow(text);
        if (StartingEpisode(text, result) != episodeId)
            throw new BuilderException("The selected episode could not be applied. Keep the original starting episode.");
        return text;
    }

    private static byte[] ReadLua(string path, CancellationToken ct)
    {
        ct.ThrowIfCancellationRequested();
        SafePaths.NoReparsePoints(path);
        using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        if (stream.Length > MaximumLuaBytes) throw new BuilderException("The episode list is too large or damaged.");
        var bytes = new byte[checked((int)stream.Length)];
        stream.ReadExactly(bytes);
        ct.ThrowIfCancellationRequested();
        return bytes;
    }

    private static string Decode(byte[] bytes) => Encoding.Latin1.GetString(bytes);

    private static string? FindChild(string parent, string name, bool? directory)
    {
        string[] matches = Directory.EnumerateFileSystemEntries(parent).Where(p =>
            Path.GetFileName(p).Equals(name, StringComparison.OrdinalIgnoreCase)).ToArray();
        if (matches.Length > 1) throw new BuilderException("Ambiguous game data names: " + name + ".");
        if (matches.Length == 0) return null;
        SafePaths.NoReparsePoints(matches[0]);
        if (directory is not null && Directory.Exists(matches[0]) != directory)
            throw new BuilderException("Unexpected file type in game data: " + name + ".");
        return matches[0];
    }

    private static string Relative(string root, string path) => Path.GetRelativePath(root, path).Replace('\\', '/');

    private static void WriteNew(string path, byte[] bytes, CancellationToken ct)
    {
        ct.ThrowIfCancellationRequested();
        using var stream = new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.None);
        stream.Write(bytes);
        stream.Flush(flushToDisk: true);
        ct.ThrowIfCancellationRequested();
    }

    private static void WriteAtomic(string path, byte[] bytes, CancellationToken ct)
    {
        string temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            WriteNew(temporary, bytes, ct);
            ct.ThrowIfCancellationRequested();
            File.Move(temporary, path, overwrite: true);
        }
        finally { if (File.Exists(temporary)) File.Delete(temporary); }
    }
}
