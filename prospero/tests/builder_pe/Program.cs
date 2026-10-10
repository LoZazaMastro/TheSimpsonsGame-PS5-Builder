// Reads CLR metadata and embedded bytes; never loads or executes the input assembly.
using System.Buffers.Binary;
using System.Reflection.Metadata;
using System.Reflection.PortableExecutable;
using System.Resources;
using System.Security.Cryptography;
using System.Text.Json;

if (args.Length != 2) throw new ArgumentException("ResourceReader <assembly> <output-directory>");
string destination = Path.GetFullPath(args[1]);
Directory.CreateDirectory(destination);
using var source = File.OpenRead(args[0]);
using var pe = new PEReader(source);
MetadataReader metadata = pe.GetMetadataReader();
var resourceDirectory = pe.PEHeaders.CorHeader!.ResourcesDirectory;
byte[] resourceBytes = pe.GetSectionData(resourceDirectory.RelativeVirtualAddress)
    .GetContent(0, resourceDirectory.Size).ToArray();
var records = new List<object>();
var markup = new List<object>();
string[] assemblyReferences = metadata.AssemblyReferences.Select(handle =>
    metadata.GetString(metadata.GetAssemblyReference(handle).Name)).Order().ToArray();
foreach (ManifestResourceHandle handle in metadata.ManifestResources)
{
    ManifestResource resource = metadata.GetManifestResource(handle);
    string name = metadata.GetString(resource.Name);
    if (!resource.Implementation.IsNil) throw new InvalidDataException("External resource: " + name);
    if (name.IndexOfAny(['/', '\\', ':']) >= 0 || name is "." or "..")
        throw new InvalidDataException("Unsafe resource name");
    int offset = checked((int)resource.Offset);
    if (offset < 0 || offset > resourceBytes.Length - 4)
        throw new InvalidDataException("Resource header bounds");
    int size = BinaryPrimitives.ReadInt32LittleEndian(resourceBytes.AsSpan(offset, 4));
    if (size < 0 || size > resourceBytes.Length - offset - 4)
        throw new InvalidDataException("Resource data bounds");
    ReadOnlySpan<byte> bytes = resourceBytes.AsSpan(offset + 4, size);
    string path = Path.Combine(destination, name);
    File.WriteAllBytes(path, bytes.ToArray());
    records.Add(new { name, bytes = size, sha256 = Convert.ToHexStringLower(SHA256.HashData(bytes)) });
    if (name.EndsWith(".g.resources", StringComparison.Ordinal))
    {
        // GetResourceData reads the stored bytes; never request deserialized values.
        using var reader = new ResourceReader(new MemoryStream(bytes.ToArray(), writable: false));
        var entries = reader.GetEnumerator();
        while (entries.MoveNext())
        {
            string key = (string)entries.Key;
            reader.GetResourceData(key, out string type, out byte[] stored);
            markup.Add(new { container = name, name = key, type, bytes = stored.Length,
                sha256 = Convert.ToHexStringLower(SHA256.HashData(stored)) });
        }
    }
}
Console.WriteLine(JsonSerializer.Serialize(new { status = "PASS", assemblyExecuted = false,
    assemblyReferences, resources = records, compiledMarkup = markup },
    new JsonSerializerOptions { WriteIndented = true }));
