namespace Simpsons.Builder;

/// <summary>
/// A user-supplied local file, copied without loading or executing it. Per the
/// requested workflow, this checks only ordinary filesystem accessibility.
/// There is deliberately no PE/version/export/digest compatibility gate here.
/// </summary>
public sealed class OptionalLibrary : IDisposable
{
    public const string Destination = "userdata/lossless/Lossless.dll";
    public string FullPath { get; }
    public FileStream Stream { get; }
    public long Length => Stream.Length;

    private OptionalLibrary(string path, FileStream stream) { FullPath = path; Stream = stream; }

    public static OptionalLibrary Open(string path)
    {
        string full = Path.GetFullPath(path);
        SafePaths.NoReparsePoints(full);
        if (!File.Exists(full) || (File.GetAttributes(full) & (FileAttributes.Directory | FileAttributes.Device)) != 0)
            throw new BuilderException("The selected Lossless.dll file could not be found. Choose it again or remove it from this build.");
        return new OptionalLibrary(full, new FileStream(full, FileMode.Open, FileAccess.Read, FileShare.Read, 1024 * 1024, FileOptions.SequentialScan));
    }

    public void Dispose() => Stream.Dispose();
}
