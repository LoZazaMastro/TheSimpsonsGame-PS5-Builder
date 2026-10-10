using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Windows.Media;

namespace Simpsons.Builder.Desktop;

public abstract class ObservableObject : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;
    protected void Changed([CallerMemberName] string? name = null) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    protected bool Set<T>(ref T field, T value, [CallerMemberName] string? name = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value)) return false;
        field = value; Changed(name); return true;
    }
}

public sealed class SettingViewModel : ObservableObject
{
    private readonly Action<string, JsonElement> change;
    private readonly Action validationChanged;
    private bool syncing;
    private JsonElement current;
    private bool enabled = true;
    private bool experimental;
    private string validationError = "";
    private string numericText = "";
    private string colorHex = "";
    public CapabilitySetting Definition { get; }
    public string Key => Definition.Key;
    public string Label => Definition.Label;
    public string Description => Definition.Description;
    public string Hint => Definition.Hint;
    public bool HasHint => Hint.Length > 0;
    public IReadOnlyList<CapabilityChoice> Choices => Definition.Choices;
    public bool IsChoice => Definition.Type == "choice";
    public bool IsBoolean => Definition.Type == "boolean";
    public bool IsNumber => Definition.Type == "number";
    public bool IsColor => Definition.Type == "color";
    public bool IsEnabled { get => enabled; internal set => Set(ref enabled, value); }
    public bool IsExperimental { get => experimental; internal set => Set(ref experimental, value); }
    public string ValidationError { get => validationError; private set { if (Set(ref validationError, value)) { Changed(nameof(HasError)); validationChanged(); } } }
    public bool HasError => ValidationError.Length > 0;
    public double Minimum => (double)(Definition.Min ?? 0);
    public double Maximum => (double)(Definition.Max ?? 1);
    public double TickFrequency => (double)(Definition.Step ?? 0.01m);
    public string RangeLabel => $"{Minimum.ToString("0.##", CultureInfo.InvariantCulture)} to {Maximum.ToString("0.##", CultureInfo.InvariantCulture)}";

    public SettingViewModel(CapabilitySetting definition, JsonElement value, Action<string, JsonElement> onChange, Action onValidationChanged)
    {
        Definition = definition; change = onChange; validationChanged = onValidationChanged;
        Synchronize(value);
    }

    public CapabilityChoice? SelectedChoice
    {
        get => Choices.FirstOrDefault(c => Capabilities.Equal(c.Value, current));
        set { if (value is not null && !Capabilities.Equal(value.Value, current)) Apply(value.Value); }
    }
    public bool IsOn { get => IsBoolean && current.GetBoolean(); set { if (IsBoolean && value != current.GetBoolean()) Apply(JsonSerializer.SerializeToElement(value)); } }
    public string OnOffLabel => IsOn ? "On" : "Off";
    public double NumericValue
    {
        get => IsNumber ? (double)current.GetDecimal() : 0;
        set
        {
            if (!IsNumber || syncing || !double.IsFinite(value)) return;
            decimal step = Definition.Step ?? 0.01m;
            decimal origin = Definition.Min ?? 0;
            decimal adjusted = Math.Clamp(origin + Math.Round(((decimal)value - origin) / step) * step, Definition.Min ?? decimal.MinValue, Definition.Max ?? decimal.MaxValue);
            if (adjusted != current.GetDecimal()) Apply(JsonSerializer.SerializeToElement(adjusted));
        }
    }
    public string NumericText
    {
        get => numericText;
        set
        {
            if (!Set(ref numericText, value) || syncing || !IsNumber) return;
            if (!decimal.TryParse(value, NumberStyles.AllowLeadingSign | NumberStyles.AllowDecimalPoint, CultureInfo.InvariantCulture, out decimal number) ||
                number < (Definition.Min ?? decimal.MinValue) || number > (Definition.Max ?? decimal.MaxValue))
            { ValidationError = "Enter a number from " + RangeLabel + "."; return; }
            if (Definition.Step is decimal step && step > 0 && (number - (Definition.Min ?? 0)) % step != 0)
            { ValidationError = "Use increments of " + step.ToString(CultureInfo.InvariantCulture) + "."; return; }
            ValidationError = "";
            if (number != current.GetDecimal()) Apply(JsonSerializer.SerializeToElement(number), preserveText: true);
        }
    }
    public string ColorHex
    {
        get => colorHex;
        set
        {
            if (!Set(ref colorHex, value) || syncing || !IsColor) return;
            string hex = value.Trim().TrimStart('#');
            if (!Regex.IsMatch(hex, "\\A[0-9a-fA-F]{6}\\z", RegexOptions.CultureInvariant))
            { ValidationError = "Enter a six-digit colour, for example #000000."; return; }
            ValidationError = "";
            JsonElement next = JsonSerializer.SerializeToElement(hex.ToLowerInvariant());
            if (!Capabilities.Equal(next, current)) Apply(next, preserveText: true);
        }
    }
    public Brush ColorBrush => IsColor ? new SolidColorBrush((Color)ColorConverter.ConvertFromString("#" + current.GetString())!) : Brushes.Transparent;
    public string CurrentLabel => Choices.FirstOrDefault(c => Capabilities.Equal(c.Value, current))?.Label ?? Capabilities.TomlValue(current);

    internal void Synchronize(JsonElement value)
    {
        syncing = true; current = value.Clone();
        if (IsNumber) numericText = value.GetDecimal().ToString("0.##", CultureInfo.InvariantCulture);
        if (IsColor) colorHex = "#" + value.GetString()!.ToUpperInvariant();
        ValidationError = ""; NotifyValue(); syncing = false;
    }
    private void Apply(JsonElement value, bool preserveText = false)
    {
        current = value.Clone();
        if (!preserveText)
        {
            if (IsNumber) numericText = current.GetDecimal().ToString("0.##", CultureInfo.InvariantCulture);
            if (IsColor) colorHex = "#" + current.GetString()!.ToUpperInvariant();
        }
        ValidationError = ""; NotifyValue();
        if (!syncing) change(Key, current);
    }
    private void NotifyValue()
    {
        foreach (string name in new[] { nameof(SelectedChoice), nameof(IsOn), nameof(OnOffLabel), nameof(NumericValue), nameof(NumericText), nameof(ColorHex), nameof(ColorBrush), nameof(CurrentLabel) }) Changed(name);
    }
}

public sealed record PresetOption(string Label, CapabilityPreset? Definition)
{
    public override string ToString() => Label;
}

public sealed class SettingGroupViewModel : ObservableObject
{
    private readonly Action<CapabilityPreset> selectPreset;
    private bool syncing;
    private PresetOption? selectedPreset;
    public CapabilityGroup Definition { get; }
    public string Id => Definition.Id;
    public string Label => Definition.Label;
    public override string ToString() => Label;
    public string Description => Definition.Description;
    public bool HasDescription => Description.Length > 0;
    public bool RequiresLossless => Definition.RequiresFile == "lossless.dll";
    public bool IsGame => Id == "game";
    public ObservableCollection<SettingViewModel> Settings { get; }
    public IReadOnlyList<PresetOption> Presets { get; }
    public bool HasPresets => Presets.Count > 1;
    public PresetOption? SelectedPreset
    {
        get => selectedPreset;
        set { if (Set(ref selectedPreset, value) && !syncing && value?.Definition is CapabilityPreset preset) selectPreset(preset); }
    }
    public SettingGroupViewModel(CapabilityGroup definition, IEnumerable<SettingViewModel> settings, IEnumerable<CapabilityPreset> presets, Action<CapabilityPreset> onPreset)
    {
        Definition = definition; Settings = new(settings); selectPreset = onPreset;
        Presets = new[] { new PresetOption("Custom", null) }.Concat(presets.Select(p => new PresetOption(p.Label, p))).ToArray();
        selectedPreset = Presets[0];
    }
    internal void RefreshPreset(IReadOnlyDictionary<string, JsonElement> values)
    {
        syncing = true;
        SelectedPreset = Presets.FirstOrDefault(p => p.Definition is not null && p.Definition.Values.All(v => Capabilities.Equal(values[v.Key], v.Value))) ?? Presets[0];
        syncing = false;
    }
}
