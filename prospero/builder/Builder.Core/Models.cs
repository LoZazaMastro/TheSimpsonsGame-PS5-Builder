using System.Globalization;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Simpsons.Builder;

public sealed class BuilderException(string message, Exception? inner = null) : Exception(message, inner);

public enum InputKind { Iso, GameDataFolder }
public sealed record BuildInput(InputKind Kind, string Path);
public sealed record BuildRequest(BuildInput Input, string OutputParent, IReadOnlyDictionary<string, JsonElement> Settings,
    GamePatchOptions? PatchOptions = null, string? LosslessDllPath = null);
public sealed record BuildProgress(string Phase, string Message, double Percent = 0,
    string? CurrentFile = null, long BytesDone = 0, long BytesTotal = 0, int FilesDone = 0, int FilesTotal = 0);
public sealed record BuildResult(string OutputDirectory, int FileCount, long GameBytes, string XboxTitleId,
    string MediaId, string DefaultXexSha256, TimeSpan Duration);
public sealed record GameIdentity(string TitleId, string MediaId, string Version, byte DiscNumber, byte DiscCount);
public sealed record GameEntry(string RelativePath, bool IsDirectory, long Length, long Offset = 0,
    string? SourcePath = null, DateTime? LastWriteUtc = null);

public static class JsonFiles
{
    public static readonly JsonSerializerOptions Options = new()
    {
        PropertyNameCaseInsensitive = true,
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        WriteIndented = true,
        Converters = { new JsonStringEnumConverter() }
    };

    public static T Read<T>(Stream stream) => JsonSerializer.Deserialize<T>(stream, Options)
        ?? throw new BuilderException("A bundled information file is empty. Download a fresh copy of the builder.");
}

public static class Format
{
    public static string Bytes(long value)
    {
        string[] units = ["B", "KB", "MB", "GB", "TB"];
        double size = value;
        int unit = 0;
        while (size >= 1024 && unit < units.Length - 1) { size /= 1024; unit++; }
        return size.ToString(unit == 0 ? "0" : "0.0", CultureInfo.InvariantCulture) + " " + units[unit];
    }
}
