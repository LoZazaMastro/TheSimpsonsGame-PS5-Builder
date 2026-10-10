using System.IO.Compression;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace Simpsons.Builder;

public sealed class PayloadFile
{
    public string Path { get; set; } = "";
    public long Bytes { get; set; }
    public string Sha256 { get; set; } = "";
}
public sealed class PayloadManifest
{
    public int SchemaVersion { get; set; }
    public string TitleId { get; set; } = "";
    public string GameTitle { get; set; } = "";
    public string BuildVersion { get; set; } = "";
    public string XboxTitleId { get; set; } = "";
    public List<PayloadFile> Files { get; set; } = [];
    public List<string> EmptyDirectories { get; set; } = [];
    public long TotalBytes => Files.Sum(f => f.Bytes);
}

public sealed class BundlePackage(PayloadManifest manifest, Capabilities capabilities, Func<Stream> openZip)
{
    public PayloadManifest Manifest { get; } = manifest;
    public Capabilities Capabilities { get; } = capabilities;
    public ZipArchive OpenZip() => new(openZip(), ZipArchiveMode.Read, leaveOpen: false);

    public void Validate()
    {
        Capabilities.ValidateManifest();
        if (Manifest.SchemaVersion != 1 || Manifest.TitleId != "PPSA74207" || Manifest.GameTitle != "The Simpsons Game" ||
            Manifest.XboxTitleId != GameSource.SimpsonsTitleId || Manifest.BuildVersion != Capabilities.BuildVersion ||
            Manifest.TitleId != Capabilities.TitleId || Manifest.Files.Count == 0)
            throw new BuilderException("The bundled PS5 files do not match this builder. Download a fresh copy.");
        var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (PayloadFile file in Manifest.Files)
        {
            SafePaths.ValidateRelative(file.Path);
            if (!names.Add(file.Path) || file.Bytes < 0 || !Regex.IsMatch(file.Sha256, "^[a-fA-F0-9]{64}$") ||
                !(file.Path == "eboot.bin" || file.Path.StartsWith("sce_sys/", StringComparison.Ordinal) ||
                  file.Path.StartsWith("sce_module/", StringComparison.Ordinal)))
                throw new BuilderException("The bundled PS5 file list is invalid.");
        }
        string[] required = ["eboot.bin", "sce_module/libc.prx", "sce_sys/param.json", "sce_sys/icon0.png", "sce_sys/pic0.dds", "sce_sys/pic1.dds", "sce_sys/snd0.at9"];
        if (required.Any(name => !Manifest.Files.Any(f => f.Path == name && f.Bytes > 0)))
            throw new BuilderException("A required PS5 title file is missing from the builder.");
        foreach (string directory in Manifest.EmptyDirectories)
            if (directory is not ("logs" or "cache" or "userdata"))
                throw new BuilderException("The bundled PS5 folder list is invalid.");
        _ = checked(Manifest.TotalBytes + 256L * 1024 * 1024);
    }

    public void ValidateTitleMetadata(string stage)
    {
        using JsonDocument document = JsonDocument.Parse(File.ReadAllText(SafePaths.Under(stage, "sce_sys/param.json")));
        JsonElement root = document.RootElement;
        if (root.GetProperty("titleId").GetString() != Manifest.TitleId ||
            root.GetProperty("downloadDataSize").GetInt32() != 256 ||
            !(root.GetProperty("contentId").GetString() ?? "").Contains("-" + Manifest.TitleId + "_", StringComparison.Ordinal))
            throw new BuilderException("The PS5 title information failed verification.");
        JsonElement localized = root.GetProperty("localizedParameters");
        foreach (JsonProperty locale in localized.EnumerateObject())
            if (locale.Value.ValueKind == JsonValueKind.Object &&
                locale.Value.TryGetProperty("titleName", out JsonElement title) && title.GetString() != Manifest.GameTitle)
                throw new BuilderException("The PS5 title name does not match this builder.");
        if (!localized.EnumerateObject().Any(p => p.Value.ValueKind == JsonValueKind.Object && p.Value.TryGetProperty("titleName", out _)))
            throw new BuilderException("The PS5 title name is missing.");
    }
}
