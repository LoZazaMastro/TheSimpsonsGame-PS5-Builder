using System.Globalization;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace Simpsons.Builder;

public sealed class CapabilityGroup
{
    public string Id { get; set; } = "";
    public string Label { get; set; } = "";
    public string Description { get; set; } = "";
    public string? RequiresFile { get; set; }
}
public sealed class CapabilityChoice
{
    public JsonElement Value { get; set; }
    public string Label { get; set; } = "";
    public string? Status { get; set; }
    public string? ApplyMode { get; set; }
    public override string ToString() => Label + (Status == "experimental" ? "  ·  Experimental" : "");
}
public sealed class CapabilityCondition
{
    public string Key { get; set; } = "";
    public JsonElement Value { get; set; }
}
public sealed class CapabilitySetting
{
    public string Key { get; set; } = "";
    public string Label { get; set; } = "";
    public string Group { get; set; } = "";
    public string Type { get; set; } = "choice";
    public JsonElement Default { get; set; }
    public string Description { get; set; } = "";
    public string Hint { get; set; } = "";
    public string Status { get; set; } = "supported";
    public string ApplyMode { get; set; } = "restart";
    public List<CapabilityChoice> Choices { get; set; } = [];
    public decimal? Min { get; set; }
    public decimal? Max { get; set; }
    public decimal? Step { get; set; }
    public CapabilityCondition? Requires { get; set; }
}
public sealed class CapabilityPreset
{
    public string Id { get; set; } = "";
    public string Label { get; set; } = "";
    public string Group { get; set; } = "";
    public Dictionary<string, JsonElement> Values { get; set; } = [];
    public override string ToString() => Label;
}

public sealed class Capabilities
{
    public int SchemaVersion { get; set; }
    public string TitleId { get; set; } = "";
    public string GameTitle { get; set; } = "";
    public string BuildVersion { get; set; } = "";
    public List<CapabilityGroup> Groups { get; set; } = [];
    public List<CapabilitySetting> Settings { get; set; } = [];
    public List<CapabilityPreset> Presets { get; set; } = [];
    public Dictionary<string, JsonElement> LockedSettings { get; set; } = [];

    public Dictionary<string, JsonElement> Defaults() => Settings.ToDictionary(s => s.Key, s => s.Default.Clone());

    // The selected preset may enable frame generation. Until the optional file
    // is supplied, all dependent switches must stay off regardless of that
    // preset; target and multiplier retain their recommended values.
    public Dictionary<string, JsonElement> Defaults(bool losslessAvailable)
    {
        Dictionary<string, JsonElement> values = Defaults();
        ApplyOptionalFileAvailability(values, wasAvailable: false, isAvailable: losslessAvailable);
        return values;
    }

    public void ApplyOptionalFileAvailability(Dictionary<string, JsonElement> values, bool wasAvailable, bool isAvailable)
    {
        foreach (CapabilitySetting setting in Settings.Where(RequiresLosslessFile))
        {
            if (!isAvailable)
                values[setting.Key] = setting.Type == "boolean" ? JsonSerializer.SerializeToElement(false) : setting.Default.Clone();
            else if (!wasAvailable && setting.Type == "boolean")
                values[setting.Key] = setting.Default.Clone();
        }
    }

    private bool RequiresLosslessFile(CapabilitySetting setting) =>
        Groups.Any(g => g.Id == setting.Group && g.RequiresFile == "lossless.dll");

    public void ValidateManifest()
    {
        if (SchemaVersion != 1 || TitleId != "PPSA74207" || GameTitle != "The Simpsons Game" || Settings.Count == 0)
            throw new BuilderException("The bundled settings information is invalid. Download a fresh copy of the builder.");
        if (Settings.Select(s => s.Key).Distinct().Count() != Settings.Count ||
            Groups.Select(g => g.Id).Distinct().Count() != Groups.Count)
            throw new BuilderException("The bundled settings contain duplicate entries.");
        if (Groups.Any(g => g.RequiresFile is not (null or "lossless.dll")))
            throw new BuilderException("The bundled settings request an unknown optional file.");
        foreach (string key in Settings.Select(s => s.Key).Concat(LockedSettings.Keys))
            if (!Regex.IsMatch(key, "^[a-z][a-z0-9_]*$", RegexOptions.CultureInvariant))
                throw new BuilderException("A bundled setting name is invalid.");
        foreach (CapabilitySetting setting in Settings)
        {
            if (!Groups.Any(g => g.Id == setting.Group) || LockedSettings.ContainsKey(setting.Key) ||
                setting.Status is not ("supported" or "experimental") ||
                setting.ApplyMode is not ("live" or "restart" or "fixed"))
                throw new BuilderException("The bundled settings contain an invalid group or policy.");
            if (setting.Requires is not null && !Settings.Any(s => s.Key == setting.Requires.Key))
                throw new BuilderException("A setting depends on an unknown setting.");
        }
        ValidateValues(Defaults());
        foreach (CapabilityPreset preset in Presets)
        {
            if (!Groups.Any(g => g.Id == preset.Group) || preset.Values.Count == 0)
                throw new BuilderException("A bundled settings preset is invalid.");
            Dictionary<string, JsonElement> values = Defaults();
            foreach ((string key, JsonElement value) in preset.Values) values[key] = value;
            ValidateValues(values);
        }
        foreach (JsonElement value in LockedSettings.Values) _ = TomlValue(value);
    }

    public void ValidateValues(IReadOnlyDictionary<string, JsonElement> values)
    {
        if (values.Count != Settings.Count || values.Keys.Any(k => !Settings.Any(s => s.Key == k)))
            throw new BuilderException("The selected settings do not match this builder. Restore the recommended settings and try again.");
        foreach (CapabilitySetting setting in Settings)
        {
            if (!values.TryGetValue(setting.Key, out JsonElement value))
                throw new BuilderException("A required setting is missing: " + setting.Label);
            bool valid = setting.Type switch
            {
                "choice" => setting.Choices.Count > 0 && setting.Choices.Any(c => Equal(c.Value, value)),
                "boolean" => value.ValueKind is JsonValueKind.True or JsonValueKind.False,
                "number" => value.ValueKind == JsonValueKind.Number && value.TryGetDecimal(out decimal n) &&
                    (!setting.Min.HasValue || n >= setting.Min.Value) &&
                    (!setting.Max.HasValue || n <= setting.Max.Value) &&
                    (!setting.Step.HasValue || (setting.Step.Value > 0 &&
                        (n - (setting.Min ?? 0)) % setting.Step.Value == 0)),
                "color" => value.ValueKind == JsonValueKind.String &&
                    Regex.IsMatch(value.GetString()!, "^[0-9a-fA-F]{6}$", RegexOptions.CultureInvariant),
                _ => false
            };
            if (!valid) throw new BuilderException("Choose a valid value for " + setting.Label + ".");
            _ = TomlValue(value);
        }
    }

    public bool IsEnabled(CapabilitySetting setting, IReadOnlyDictionary<string, JsonElement> values) =>
        setting.Requires is null || (values.TryGetValue(setting.Requires.Key, out JsonElement value) && Equal(value, setting.Requires.Value));

    public void ValidateOptionalFileChoices(IReadOnlyDictionary<string, JsonElement> values, bool losslessAvailable)
    {
        if (losslessAvailable) return;
        Dictionary<string, JsonElement> unavailable = Defaults(losslessAvailable: false);
        foreach (CapabilitySetting setting in Settings.Where(RequiresLosslessFile))
            if (!Equal(values[setting.Key], unavailable[setting.Key]))
                throw new BuilderException("Choose your Lossless.dll file to use the frame generation options, or turn frame generation off and restore its target and multiplier.");
    }

    public string WriteToml(IReadOnlyDictionary<string, JsonElement> values)
    {
        ValidateValues(values);
        var output = new StringBuilder("# The Simpsons Game — PS5\n# Created by The Simpsons Game PS5 Builder.\n\n");
        foreach ((string key, JsonElement value) in LockedSettings.OrderBy(p => p.Key, StringComparer.Ordinal))
            output.Append(key).Append(" = ").Append(TomlValue(value)).Append('\n');
        foreach (CapabilityGroup group in Groups)
        {
            output.Append("\n# ").Append(group.Label.Replace('\n', ' ').Replace('\r', ' ')).Append('\n');
            foreach (CapabilitySetting setting in Settings.Where(s => s.Group == group.Id))
                output.Append(setting.Key).Append(" = ").Append(TomlValue(values[setting.Key])).Append('\n');
        }
        return output.ToString();
    }

    public IEnumerable<string> ExperimentalSelections(IReadOnlyDictionary<string, JsonElement> values) =>
        Settings.Where(s => IsEnabled(s, values) && ((s.Status == "experimental" &&
            (s.Type == "boolean" ? values[s.Key].GetBoolean() : !Equal(s.Default, values[s.Key]))) ||
            s.Choices.Any(c => c.Status == "experimental" && Equal(c.Value, values[s.Key])))).Select(s => s.Label);

    public static bool Equal(JsonElement a, JsonElement b) => a.ValueKind == b.ValueKind &&
        (a.ValueKind == JsonValueKind.Number ? a.GetDecimal() == b.GetDecimal() : a.GetRawText() == b.GetRawText());

    public static string TomlValue(JsonElement value) => value.ValueKind switch
    {
        JsonValueKind.String => "\"" + Escape(value.GetString()!) + "\"",
        JsonValueKind.True => "true",
        JsonValueKind.False => "false",
        JsonValueKind.Number when value.TryGetDecimal(out decimal number) => number.ToString(CultureInfo.InvariantCulture),
        _ => throw new BuilderException("A bundled setting has an unsupported value.")
    };

    private static string Escape(string value)
    {
        var result = new StringBuilder();
        foreach (char c in value)
        {
            if (c is '"' or '\\') result.Append('\\').Append(c);
            else if (c < 32 || c == 127) result.Append("\\u").Append(((int)c).ToString("X4", CultureInfo.InvariantCulture));
            else result.Append(c);
        }
        return result.ToString();
    }
}
