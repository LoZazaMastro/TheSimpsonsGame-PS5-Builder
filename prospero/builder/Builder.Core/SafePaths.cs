namespace Simpsons.Builder;

public static class SafePaths
{
    private static readonly HashSet<string> Devices = new(StringComparer.OrdinalIgnoreCase)
    {
        "CON", "PRN", "AUX", "NUL", "CLOCK$", "CONIN$", "CONOUT$",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    public static string ValidateComponent(string name)
    {
        if (string.IsNullOrEmpty(name) || name is "." or ".." || name.Length > 255 ||
            name.EndsWith(' ') || name.EndsWith('.') ||
            name.Any(c => c < 32 || c is '<' or '>' or ':' or '"' or '/' or '\\' or '|' or '?' or '*' or '\u007f') ||
            Devices.Contains(name.Split('.')[0]))
            throw new BuilderException($"The game contains an unsafe or unsupported file name: {Printable(name)}. The source has not been changed.");
        return name;
    }

    public static string ValidateRelative(string relative)
    {
        if (string.IsNullOrEmpty(relative) || relative.StartsWith('/') || relative.Contains('\\') || relative.Length > 2048)
            throw new BuilderException("A file path in the input is invalid.");
        foreach (string component in relative.Split('/')) ValidateComponent(component);
        return relative;
    }

    public static string Under(string root, string relative)
    {
        ValidateRelative(relative);
        string result = Path.GetFullPath(Path.Combine(root, relative.Replace('/', Path.DirectorySeparatorChar)));
        if (!IsWithin(result, root)) throw new BuilderException("A file path points outside the output folder.");
        return result;
    }

    public static bool IsWithin(string path, string root)
    {
        string canonicalRoot = Path.TrimEndingDirectorySeparator(Path.GetFullPath(root));
        string canonicalPath = Path.GetFullPath(path);
        return canonicalPath.Equals(canonicalRoot, StringComparison.OrdinalIgnoreCase) ||
               canonicalPath.StartsWith(canonicalRoot + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase);
    }

    public static void NoReparsePoints(string path)
    {
        string? current = Path.GetFullPath(path);
        while (current is not null)
        {
            if ((File.Exists(current) || Directory.Exists(current)) &&
                (File.GetAttributes(current) & FileAttributes.ReparsePoint) != 0)
                throw new BuilderException("Choose a normal local folder or drive. Symbolic links and junctions are not supported: " + current);
            current = Path.GetDirectoryName(current);
        }
    }

    public static string CanonicalGamePath(string relative)
    {
        string[] parts = ValidateRelative(relative).Split('/');
        if (parts[0].Equals("default.xex", StringComparison.OrdinalIgnoreCase)) parts[0] = "default.xex";
        if (parts[0].Equals("movies", StringComparison.OrdinalIgnoreCase)) parts[0] = "movies";
        return string.Join('/', parts);
    }

    private static string Printable(string value) => new(value.Select(c => char.IsControl(c) ? '?' : c).Take(160).ToArray());
}
