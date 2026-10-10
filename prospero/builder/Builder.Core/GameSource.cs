using System.Buffers.Binary;
using System.Collections;
using System.Text;

namespace Simpsons.Builder;

/// <summary>
/// Newly written C# reader for the XDVDFS on-disc format. The upstream
/// extract-xiso C source supplied format/partition references and is used as an
/// external differential test, not translated, linked or embedded here.
/// No guest code is executed. Child pointers are DWORD offsets, not sectors.
/// Bounds, cycles and Windows file-name collisions are rejected before writing.
/// See THIRD-PARTY-NOTICES.txt for the reference implementation credit.
/// </summary>
public sealed class GameSource : IDisposable
{
    // The retail game identity, also documented in the upstream README.
    // Media IDs, executable versions, region masks and language folders vary
    // between disc editions and are not used as compatibility allowlists.
    public const string SimpsonsTitleId = "45410809";
    private const int SectorSize = 2048;
    private const int MaximumEntries = 200_000;
    private const int MaximumDepth = 64;
    private const int MaximumDirectoryBytes = 16 * 1024 * 1024;
    private static readonly byte[] Magic = Encoding.ASCII.GetBytes("MICROSOFT*XBOX*MEDIA");
    // These are the offsets used by upstream extract-xiso's verify_xiso.
    private static readonly long[] PartitionOffsets = [0, 0x0FD90000, 0x02080000, 0x18300000];
    private FileStream? iso;
    private readonly HashSet<string> paths = new(StringComparer.OrdinalIgnoreCase);
    private readonly HashSet<(long Offset, long Size)> directoryTables = [];
    private readonly List<GameEntry> entries = [];
    private readonly Dictionary<string, GameEntry> entriesByPath = new(StringComparer.Ordinal);
    private Action<string>? log;
    public IReadOnlyList<GameEntry> Entries => entries;
    public GameIdentity Identity { get; private set; } = new("", "", "", 0, 0);
    public long TotalBytes { get; private set; }
    public int FileCount => entries.Count(e => !e.IsDirectory);
    public long PartitionOffset { get; private set; }

    private GameSource() { }

    public static GameSource Open(BuildInput input, string expectedTitleId, CancellationToken cancellationToken,
        IProgress<BuildProgress>? progress = null, Action<string>? log = null)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var source = new GameSource { log = log };
        try
        {
            string path = Path.GetFullPath(input.Path);
            log?.Invoke(DiagnosticSession.FormatEvent("source.open.start", new { input.Kind, path, access = "read-only", expectedTitleId }));
            if (!string.Equals(expectedTitleId, SimpsonsTitleId, StringComparison.OrdinalIgnoreCase))
            {
                log?.Invoke(DiagnosticSession.FormatEvent("source.identity.configuration-error", new { expectedTitleId, requiredTitleId = SimpsonsTitleId }));
                throw new BuilderException("This builder contains an incorrect Xbox 360 game identity. Download the latest builder; your game files have not been changed.");
            }
            SafePaths.NoReparsePoints(path);
            progress?.Report(new("Inspecting", "Checking your game files…", 1));
            if (input.Kind == InputKind.Iso)
            {
                if (!File.Exists(path)) throw new BuilderException("The selected ISO could not be found. Choose the file again.");
                source.iso = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read, 1024 * 1024, FileOptions.RandomAccess);
                log?.Invoke(DiagnosticSession.FormatEvent("source.iso.accessible", new { path, bytes = source.iso.Length, source.iso.CanRead, source.iso.CanSeek }));
                source.ReadIso(cancellationToken);
            }
            else
            {
                if (!Directory.Exists(path)) throw new BuilderException("The selected game data folder could not be found.");
                log?.Invoke(DiagnosticSession.FormatEvent("source.folder.accessible", new { root = path }));
                source.ReadFolder(path, "", 0, cancellationToken);
            }
            GameEntry? xex = source.entries.SingleOrDefault(e => e.RelativePath == "default.xex" && !e.IsDirectory);
            if (xex is null || !source.entries.Any(e => e.RelativePath == "movies" && e.IsDirectory))
                throw new BuilderException("This is not a complete copy of The Simpsons Game. The game data must contain default.xex and a movies folder at its root.");
            if (!source.entries.Any(e => !e.IsDirectory && e.RelativePath.StartsWith("movies/", StringComparison.Ordinal)))
                throw new BuilderException("The movies folder is empty. Choose a complete Xbox 360 game dump.");
            source.Identity = source.ReadXex(xex, cancellationToken);
            log?.Invoke(DiagnosticSession.FormatEvent("source.identity", source.Identity));
            bool accepted = source.Identity.TitleId.Equals(SimpsonsTitleId, StringComparison.OrdinalIgnoreCase);
            log?.Invoke(DiagnosticSession.FormatEvent("source.identity.decision", new
            {
                actualTitleId = source.Identity.TitleId, expectedTitleId = SimpsonsTitleId, accepted,
                source.Identity.MediaId, source.Identity.Version,
                mediaIdRestricted = false, executableVersionRestricted = false,
                regionRestricted = false, languageRestricted = false,
                reason = accepted ? "the-simpsons-game-title-matched" : "xbox-title-id-mismatch"
            }));
            if (!accepted)
                throw new BuilderException($"The selected files identify Xbox 360 title {source.Identity.TitleId}; The Simpsons Game uses {SimpsonsTitleId}. Select its ISO or extracted gamedata folder. Disc region, language and Media ID are not restricted.");
            progress?.Report(new("Inspecting", $"The Simpsons Game found · {source.FileCount:N0} files · {Format.Bytes(source.TotalBytes)}", 6));
            log?.Invoke(DiagnosticSession.FormatEvent("source.open.complete", new { source.FileCount, source.TotalBytes, source.PartitionOffset }));
            return source;
        }
        catch (Exception e) { log?.Invoke(DiagnosticSession.FormatEvent("source.open.failed", new { input.Kind, input.Path, exception = e.ToString() })); source.Dispose(); throw; }
    }

    private void ReadIso(CancellationToken token)
    {
        byte[] descriptor = new byte[SectorSize];
        bool found = false;
        foreach (long partition in PartitionOffsets)
        {
            token.ThrowIfCancellationRequested();
            long descriptorOffset = partition + 32L * SectorSize;
            if (descriptorOffset > iso!.Length - descriptor.Length)
            {
                log?.Invoke(DiagnosticSession.FormatEvent("source.partition.probe", new { partition, descriptorOffset, result = "outside-image" }));
                continue;
            }
            ReadAt(descriptorOffset, descriptor);
            if (!descriptor.AsSpan(0, Magic.Length).SequenceEqual(Magic))
            {
                log?.Invoke(DiagnosticSession.FormatEvent("source.partition.probe", new { partition, descriptorOffset, result = "no-XDVDFS-marker" }));
                continue;
            }
            if (!descriptor.AsSpan(SectorSize - Magic.Length).SequenceEqual(Magic))
            {
                log?.Invoke(DiagnosticSession.FormatEvent("source.partition.probe", new { partition, descriptorOffset, result = "trailing-marker-mismatch" }));
                throw new BuilderException("The ISO volume information is damaged. Create a fresh dump of your disc and try again.");
            }
            PartitionOffset = partition;
            uint rootSector = BinaryPrimitives.ReadUInt32LittleEndian(descriptor.AsSpan(20));
            uint rootSize = BinaryPrimitives.ReadUInt32LittleEndian(descriptor.AsSpan(24));
            log?.Invoke(DiagnosticSession.FormatEvent("source.partition.selected", new { partition, descriptorOffset, rootSector, rootSize, rootByteOffset = checked(partition + (long)rootSector * SectorSize), sectorSize = SectorSize }));
            if (rootSector == 0 || rootSize == 0) throw new BuilderException("The ISO contains an empty game partition.");
            ReadDirectory(rootSector, rootSize, "", 0, token);
            found = true;
            break;
        }
        if (!found)
            throw new BuilderException("No Xbox 360 game partition was found. Choose an extracted XISO or a complete Xbox 360 ISO dump; compressed archives and split images are not supported.");
    }

    private void ReadDirectory(uint sector, uint size, string parent, int depth, CancellationToken token)
    {
        token.ThrowIfCancellationRequested();
        if (depth > MaximumDepth || size > MaximumDirectoryBytes)
            throw new BuilderException("The ISO directory structure exceeds the supported limits or is damaged.");
        if (size == 0) return;
        long offset = checked(PartitionOffset + (long)sector * SectorSize);
        log?.Invoke(DiagnosticSession.FormatEvent("source.directory", new { parent, depth, sector, offset, bytes = size }));
        RequireRange(offset, size);
        if (!directoryTables.Add((offset, size))) throw new BuilderException("The ISO contains a repeated or circular directory. No output has been created.");
        var data = new byte[size];
        ReadAt(offset, data);
        if (size >= 2 && BinaryPrimitives.ReadUInt16LittleEndian(data) == ushort.MaxValue) return; // Empty directory, as in extract-xiso.

        var pending = new Stack<int>();
        var visited = new HashSet<int>();
        var occupied = new BitArray((data.Length + 3) / 4);
        pending.Push(0);
        while (pending.Count > 0)
        {
            token.ThrowIfCancellationRequested();
            int position = pending.Pop();
            if (position < 0 || position > data.Length - 14 || !visited.Add(position))
                throw new BuilderException("The ISO contains an invalid or circular file table.");
            var record = data.AsSpan(position);
            ushort left = BinaryPrimitives.ReadUInt16LittleEndian(record);
            ushort right = BinaryPrimitives.ReadUInt16LittleEndian(record[2..]);
            uint start = BinaryPrimitives.ReadUInt32LittleEndian(record[4..]);
            uint length = BinaryPrimitives.ReadUInt32LittleEndian(record[8..]);
            byte attributes = record[12];
            int nameLength = record[13];
            int recordEnd = checked(position + 14 + nameLength);
            if (nameLength == 0 || recordEnd > data.Length || record[..(14 + nameLength)].Length > SectorSize - position % SectorSize)
                throw new BuilderException("The ISO contains a truncated file name or directory entry.");
            for (int word = position / 4; word < (recordEnd + 3) / 4; word++)
            {
                if (occupied[word]) throw new BuilderException("The ISO contains overlapping directory entries.");
                occupied[word] = true;
            }
            ReadOnlySpan<byte> nameBytes = record.Slice(14, nameLength);
            // Disc names are byte strings. Latin-1 preserves every byte instead of
            // silently replacing high bytes with '?' or changing the file name.
            string name = SafePaths.ValidateComponent(Encoding.Latin1.GetString(nameBytes));
            string relative = SafePaths.CanonicalGamePath(parent.Length == 0 ? name : parent + "/" + name);
            bool isDirectory = (attributes & 0x10) != 0;
            long fileOffset = checked(PartitionOffset + (long)start * SectorSize);
            RequireRange(fileOffset, length);
            Add(new(relative, isDirectory, isDirectory ? 0 : length, fileOffset));
            if (isDirectory && length > 0) ReadDirectory(start, length, relative, depth + 1, token);
            if (right != 0) pending.Push(right * 4);
            if (left != 0) pending.Push(left * 4);
        }
    }

    private void ReadFolder(string root, string parent, int depth, CancellationToken token)
    {
        token.ThrowIfCancellationRequested();
        if (depth > MaximumDepth) throw new BuilderException("The game folder has too many nested directories.");
        foreach (string item in Directory.EnumerateFileSystemEntries(root).OrderBy(Path.GetFileName, StringComparer.Ordinal))
        {
            token.ThrowIfCancellationRequested();
            FileAttributes attributes = File.GetAttributes(item);
            if ((attributes & FileAttributes.ReparsePoint) != 0)
                throw new BuilderException("The game data contains a symbolic link or junction. Choose a normal extracted game folder.");
            string name = SafePaths.ValidateComponent(Path.GetFileName(item));
            string relative = SafePaths.CanonicalGamePath(parent.Length == 0 ? name : parent + "/" + name);
            bool directory = (attributes & FileAttributes.Directory) != 0;
            var info = new FileInfo(item);
            Add(new(relative, directory, directory ? 0 : info.Length, SourcePath: item,
                LastWriteUtc: directory ? null : info.LastWriteTimeUtc));
            if (directory) ReadFolder(item, relative, depth + 1, token);
        }
    }

    private void Add(GameEntry entry)
    {
        if (entries.Count >= MaximumEntries) throw new BuilderException("The game contains too many files or folders.");
        if (!paths.Add(entry.RelativePath)) throw new BuilderException("The game contains duplicate file names that would collide on Windows: " + entry.RelativePath);
        TotalBytes = checked(TotalBytes + entry.Length);
        entries.Add(entry);
        entriesByPath.Add(entry.RelativePath, entry);
        log?.Invoke(DiagnosticSession.FormatEvent("source.entry", new { entry.RelativePath, entry.IsDirectory, entry.Length, entry.Offset, entry.LastWriteUtc }));
    }

    private void RequireRange(long offset, long size)
    {
        if (offset < PartitionOffset || size < 0 || offset > iso!.Length || size > iso.Length - offset)
            throw new BuilderException("The ISO is incomplete: a game file extends beyond the end of the image.");
    }

    private void ReadAt(long offset, Span<byte> data)
    {
        iso!.Position = offset;
        try { iso.ReadExactly(data); }
        catch (EndOfStreamException e) { throw new BuilderException("The ISO ended unexpectedly while reading game data.", e); }
    }

    public Stream OpenFile(GameEntry entry)
    {
        if (entry.IsDirectory || !entriesByPath.TryGetValue(entry.RelativePath, out GameEntry? known) || known != entry)
            throw new ArgumentException("Not a file in this source.", nameof(entry));
        if (iso is not null) return new SliceStream(iso, entry.Offset, entry.Length);
        SafePaths.NoReparsePoints(entry.SourcePath!);
        var info = new FileInfo(entry.SourcePath!);
        if (!info.Exists || info.Length != entry.Length || info.LastWriteTimeUtc != entry.LastWriteUtc)
            throw new BuilderException("A source file changed while building. Close applications using the game files, then try again: " + entry.RelativePath);
        return new FileStream(entry.SourcePath!, FileMode.Open, FileAccess.Read, FileShare.Read, 1024 * 1024, FileOptions.SequentialScan);
    }

    private GameIdentity ReadXex(GameEntry entry, CancellationToken token)
    {
        using Stream stream = OpenFile(entry);
        if (entry.Length < 24) throw new BuilderException("default.xex is empty or damaged.");
        var header = new byte[24];
        stream.ReadExactly(header);
        if (!header.AsSpan(0, 4).SequenceEqual("XEX2"u8))
            throw new BuilderException("default.xex is not an Xbox 360 XEX2 executable. Choose the Xbox 360 version of The Simpsons Game.");
        uint dataOffset = BinaryPrimitives.ReadUInt32BigEndian(header.AsSpan(8));
        uint headerCount = BinaryPrimitives.ReadUInt32BigEndian(header.AsSpan(20));
        log?.Invoke(DiagnosticSession.FormatEvent("source.xex.header", new { entry.Length, dataOffset, optionalHeaderCount = headerCount }));
        if (headerCount > 4096 || 24L + headerCount * 8L > entry.Length || dataOffset > entry.Length || dataOffset < 24L + headerCount * 8L)
            throw new BuilderException("default.xex has invalid executable header bounds.");
        var optionalHeaders = new byte[checked((int)headerCount * 8)];
        stream.ReadExactly(optionalHeaders);
        uint? executionOffset = null;
        for (int i = 0; i < optionalHeaders.Length; i += 8)
        {
            token.ThrowIfCancellationRequested();
            if (BinaryPrimitives.ReadUInt32BigEndian(optionalHeaders.AsSpan(i)) != 0x00040006) continue;
            if (executionOffset.HasValue) throw new BuilderException("default.xex contains duplicate game identity headers.");
            executionOffset = BinaryPrimitives.ReadUInt32BigEndian(optionalHeaders.AsSpan(i + 4));
        }
        if (!executionOffset.HasValue || executionOffset < 24L + headerCount * 8L || executionOffset > dataOffset - 24L)
            throw new BuilderException("The Xbox 360 game identity could not be read from default.xex.");
        stream.Position = executionOffset.Value;
        log?.Invoke(DiagnosticSession.FormatEvent("source.xex.execution-info", new { offset = executionOffset.Value, bytes = 24 }));
        var identity = new byte[24];
        stream.ReadExactly(identity);
        return new GameIdentity(BinaryPrimitives.ReadUInt32BigEndian(identity.AsSpan(12)).ToString("X8"),
            BinaryPrimitives.ReadUInt32BigEndian(identity).ToString("X8"),
            BinaryPrimitives.ReadUInt32BigEndian(identity.AsSpan(4)).ToString("X8"), identity[18], identity[19]);
    }

    public void Dispose() { iso?.Dispose(); iso = null; log?.Invoke("source.closed"); log = null; }

    private sealed class SliceStream(Stream source, long start, long length) : Stream
    {
        private long position;
        public override bool CanRead => true;
        public override bool CanSeek => true;
        public override bool CanWrite => false;
        public override long Length => length;
        public override long Position { get => position; set => Seek(value, SeekOrigin.Begin); }
        public override void Flush() { }
        public override int Read(byte[] buffer, int offset, int count) => Read(buffer.AsSpan(offset, count));
        public override int Read(Span<byte> buffer)
        {
            int count = (int)Math.Min(buffer.Length, length - position);
            source.Position = start + position;
            int read = source.Read(buffer[..count]);
            position += read;
            return read;
        }
        public override long Seek(long offset, SeekOrigin origin)
        {
            long value = checked((origin switch { SeekOrigin.Begin => 0, SeekOrigin.Current => position, SeekOrigin.End => length, _ => throw new ArgumentOutOfRangeException(nameof(origin)) }) + offset);
            if (value < 0 || value > length) throw new IOException("Attempt to read beyond the game file.");
            position = value;
            return position;
        }
        public override void SetLength(long value) => throw new NotSupportedException();
        public override void Write(byte[] buffer, int offset, int count) => throw new NotSupportedException();
        // The source owns the ISO handle. Disposing one file slice must not close it.
    }
}
