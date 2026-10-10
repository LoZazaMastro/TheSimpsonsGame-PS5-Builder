using System.Buffers.Binary;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using Simpsons.Builder;

namespace Simpsons.Builder.Tests;

internal sealed record IsoFixture(string Path, long Partition, long RootOffset, IReadOnlyDictionary<string, long> Records,
    IReadOnlyDictionary<string, long> FileOffsets);

internal static class Fixtures
{
    internal static byte[] Xex(uint title = 0x45410809, uint mediaId = 0x11223344,
        uint version = 0x00000100, uint baseVersion = 0, uint? region = null,
        byte discNumber = 1, byte discCount = 1)
    {
        int dataOffset = region.HasValue ? 0x280 : 128;
        var data = new byte[region.HasValue ? 1024 : 256]; "XEX2"u8.CopyTo(data);
        BinaryPrimitives.WriteUInt32BigEndian(data.AsSpan(8), (uint)dataOffset);
        BinaryPrimitives.WriteUInt32BigEndian(data.AsSpan(20), 1);
        BinaryPrimitives.WriteUInt32BigEndian(data.AsSpan(24), 0x00040006);
        BinaryPrimitives.WriteUInt32BigEndian(data.AsSpan(28), 64);
        BinaryPrimitives.WriteUInt32BigEndian(data.AsSpan(64), mediaId);
        BinaryPrimitives.WriteUInt32BigEndian(data.AsSpan(68), version);
        BinaryPrimitives.WriteUInt32BigEndian(data.AsSpan(72), baseVersion);
        BinaryPrimitives.WriteUInt32BigEndian(data.AsSpan(76), title);
        data[80] = 2; data[82] = discNumber; data[83] = discCount;
        if (region.HasValue)
        {
            // Layout from xex2_security_info in the bundled rexglue SDK:
            // security header offset is XEX + 0x10; region is security + 0x178.
            // These are unencrypted synthetic bytes, not an executable retail XEX.
            const int securityOffset = 128;
            BinaryPrimitives.WriteUInt32BigEndian(data.AsSpan(16), securityOffset);
            BinaryPrimitives.WriteUInt32BigEndian(data.AsSpan(securityOffset), 0x184);
            BinaryPrimitives.WriteUInt32BigEndian(data.AsSpan(securityOffset + 0x178), region.Value);
        }
        "SYNTHETIC TEST DATA; NOT A GAME EXECUTABLE"u8.CopyTo(data.AsSpan(dataOffset));
        return data;
    }

    internal static Dictionary<string, byte[]> GameFiles() => new(StringComparer.Ordinal)
    {
        ["DEFAULT.XEX"] = Xex(),
        ["MOVIES/italian/ealogo.vp6"] = "synthetic italian logo"u8.ToArray(),
        ["MOVIES/spanish/foxlogo.vp6"] = "synthetic spanish logo"u8.ToArray(),
        ["MOVIES/italian/intro.vp6"] = "synthetic movie"u8.ToArray(),
        ["archive.bin"] = Enumerable.Range(0, 2 * 1024 * 1024 + 37).Select(i => (byte)(i * 37)).ToArray(),
        ["simpsons_gameflow.lua"] = Encoding.Latin1.GetBytes("-- test fixture\r\nlocal game = NewGame()\r\n  episode = NewEpisode(game, \"LAND_OF_CHOCOLATE\")\r\n  episode:SetDefault()\r\n  episode = NewEpisode(game, \"BARTMAN_BEGINS\")\r\n")
    };

    internal static string Folder(string path, Dictionary<string, byte[]>? files = null)
    {
        Directory.CreateDirectory(path);
        foreach ((string name, byte[] content) in files ?? GameFiles())
        {
            string output = System.IO.Path.Combine(path, name.Replace('/', System.IO.Path.DirectorySeparatorChar));
            Directory.CreateDirectory(System.IO.Path.GetDirectoryName(output)!);
            File.WriteAllBytes(output, content);
        }
        return path;
    }

    private sealed class Node(string name, string path, bool directory, byte[]? bytes = null)
    {
        internal string Name = name;
        internal string Relative = path;
        internal bool Directory = directory;
        internal byte[] Bytes = bytes ?? [];
        internal List<Node> Children = [];
        internal uint Sector;
        internal int TableSize;
        internal int Position;
        internal Node? Left;
        internal Node? Right;
    }

    internal static IsoFixture Iso(string path, long partition = 0, Dictionary<string, byte[]>? files = null)
    {
        var root = new Node("", "", true);
        foreach ((string relative, byte[] bytes) in files ?? GameFiles())
        {
            string[] parts = relative.Split('/');
            Node current = root;
            for (int i = 0; i < parts.Length; i++)
            {
                Node? existing = current.Children.FirstOrDefault(n => n.Name == parts[i]);
                if (existing is null)
                {
                    existing = new Node(parts[i], string.Join('/', parts.Take(i + 1)), i < parts.Length - 1, i == parts.Length - 1 ? bytes : null);
                    current.Children.Add(existing);
                }
                current = existing;
            }
        }
        var directories = new List<Node>();
        var regular = new List<Node>();
        void Collect(Node node)
        {
            if (node.Directory) { directories.Add(node); foreach (Node child in node.Children) Collect(child); }
            else regular.Add(node);
        }
        Collect(root);
        Node? Tree(List<Node> nodes, int low, int high)
        {
            if (low >= high) return null;
            int mid = (low + high) / 2;
            Node node = nodes[mid]; node.Left = Tree(nodes, low, mid); node.Right = Tree(nodes, mid + 1, high); return node;
        }
        foreach (Node directory in directories)
        {
            List<Node> sorted = directory.Children.OrderBy(n => n.Name, StringComparer.OrdinalIgnoreCase).ToList();
            Node? top = Tree(sorted, 0, sorted.Count);
            var order = new List<Node>();
            void Visit(Node? node) { if (node is null) return; order.Add(node); Visit(node.Left); Visit(node.Right); }
            Visit(top); directory.Children = order;
            int offset = 0;
            foreach (Node node in order)
            {
                int length = (14 + Encoding.Latin1.GetByteCount(node.Name) + 3) & ~3;
                if (offset % 2048 + length > 2048) offset = (offset + 2047) & ~2047;
                node.Position = offset; offset += length;
            }
            directory.TableSize = Math.Max(2048, (offset + 2047) & ~2047);
        }
        uint sector = 40;
        foreach (Node directory in directories) { directory.Sector = sector; sector += (uint)directory.TableSize / 2048; }
        foreach (Node file in regular) { file.Sector = sector; sector += (uint)(file.Bytes.Length + 2047) / 2048; }
        Directory.CreateDirectory(System.IO.Path.GetDirectoryName(path)!);
        using var output = new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.None);
        // Full-disc fixtures leave every upstream candidate probe in bounds.
        // extract-xiso aborts on an EOF at an earlier candidate, even when a
        // later (lower) XGD3 candidate is valid. Files remain sparse on disk.
        output.SetLength(Math.Max(checked(partition + (sector + 1L) * 2048), partition == 0 ? 0 : 0x18300000L + 34L * 2048));
        var descriptor = new byte[2048];
        "MICROSOFT*XBOX*MEDIA"u8.CopyTo(descriptor); "MICROSOFT*XBOX*MEDIA"u8.CopyTo(descriptor.AsSpan(2028));
        BinaryPrimitives.WriteUInt32LittleEndian(descriptor.AsSpan(20), root.Sector);
        BinaryPrimitives.WriteUInt32LittleEndian(descriptor.AsSpan(24), (uint)root.TableSize);
        output.Position = partition + 32L * 2048; output.Write(descriptor);
        var records = new Dictionary<string, long>(StringComparer.Ordinal);
        var offsets = new Dictionary<string, long>(StringComparer.Ordinal);
        foreach (Node directory in directories)
        {
            var data = Enumerable.Repeat((byte)0xff, directory.TableSize).ToArray();
            foreach (Node node in directory.Children)
            {
                Span<byte> record = data.AsSpan(node.Position);
                BinaryPrimitives.WriteUInt16LittleEndian(record, (ushort)((node.Left?.Position ?? 0) / 4));
                BinaryPrimitives.WriteUInt16LittleEndian(record[2..], (ushort)((node.Right?.Position ?? 0) / 4));
                BinaryPrimitives.WriteUInt32LittleEndian(record[4..], node.Sector);
                BinaryPrimitives.WriteUInt32LittleEndian(record[8..], node.Directory ? (uint)node.TableSize : (uint)node.Bytes.Length);
                record[12] = node.Directory ? (byte)0x10 : (byte)0x20;
                byte[] name = Encoding.Latin1.GetBytes(node.Name); record[13] = (byte)name.Length; name.CopyTo(record[14..]);
                records[node.Relative] = partition + directory.Sector * 2048L + node.Position;
            }
            output.Position = partition + directory.Sector * 2048L; output.Write(data);
        }
        foreach (Node file in regular)
        {
            output.Position = partition + file.Sector * 2048L; offsets[file.Relative] = output.Position; output.Write(file.Bytes);
        }
        return new(path, partition, partition + root.Sector * 2048L, records, offsets);
    }

    internal static void WriteAt(string file, long offset, ReadOnlySpan<byte> bytes)
    {
        using var stream = new FileStream(file, FileMode.Open, FileAccess.Write, FileShare.None); stream.Position = offset; stream.Write(bytes);
    }
    internal static byte[] Little32(uint value) { var bytes = new byte[4]; BinaryPrimitives.WriteUInt32LittleEndian(bytes, value); return bytes; }

    internal static (BundlePackage Bundle, byte[] Zip) Bundle(Capabilities capabilities, string? metadataTitle = null,
        string? extraName = null, bool badHash = false, bool duplicate = false)
    {
        byte[] metadata = JsonSerializer.SerializeToUtf8Bytes(new
        {
            titleId = "PPSA74207", contentId = "UP9000-PPSA74207_00-SIMPSONSRECOMP01", downloadDataSize = 256,
            localizedParameters = new Dictionary<string, object> { ["defaultLanguage"] = "en-US", ["en-US"] = new { titleName = metadataTitle ?? "The Simpsons Game" } }
        });
        var files = new Dictionary<string, byte[]>
        {
            ["eboot.bin"] = "SYNTHETIC PS5 PAYLOAD FIXTURE; NEVER EXECUTE"u8.ToArray(),
            ["sce_module/libc.prx"] = "fixture library"u8.ToArray(), ["sce_sys/param.json"] = metadata,
            ["sce_sys/icon0.png"] = [1, 2, 3], ["sce_sys/pic0.dds"] = [4, 5, 6],
            ["sce_sys/pic1.dds"] = [7, 8, 9], ["sce_sys/snd0.at9"] = [10, 11, 12]
        };
        var manifest = new PayloadManifest { SchemaVersion = 1, TitleId = "PPSA74207", GameTitle = "The Simpsons Game", XboxTitleId = "45410809", BuildVersion = capabilities.BuildVersion, EmptyDirectories = ["cache", "logs", "userdata"] };
        using var buffer = new MemoryStream();
        using (var zip = new ZipArchive(buffer, ZipArchiveMode.Create, true))
        {
            foreach ((string name, byte[] bytes) in files)
            {
                using Stream stream = zip.CreateEntry(name).Open(); stream.Write(bytes);
                manifest.Files.Add(new PayloadFile { Path = name, Bytes = bytes.Length, Sha256 = Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant() });
            }
            if (extraName is not null) { using Stream extra = zip.CreateEntry(extraName).Open(); extra.WriteByte(42); }
            if (duplicate) { using Stream repeated = zip.CreateEntry("EBOOT.BIN").Open(); repeated.Write(files["eboot.bin"]); }
        }
        if (badHash) manifest.Files[0].Sha256 = new string('0', 64);
        byte[] zipData = buffer.ToArray();
        return (new BundlePackage(manifest, capabilities, () => new MemoryStream(zipData, false)), zipData);
    }
}
