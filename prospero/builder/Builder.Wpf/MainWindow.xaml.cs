using Microsoft.Win32;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;

namespace Simpsons.Builder.Desktop;

public sealed record SummaryRow(string Label, string Value);
public sealed record EpisodeOption(string Label, EpisodeChoice? Choice)
{
    public override string ToString() => Label;
}

public partial class MainWindow : Window, INotifyPropertyChanged
{
    private readonly BundlePackage bundle;
    private readonly DiagnosticSession diagnostics;
    private readonly Dictionary<string, JsonElement> settings;
    private readonly Dictionary<string, SettingViewModel> settingModels = [];
    private readonly DispatcherTimer logRefresh = new() { Interval = TimeSpan.FromMilliseconds(500) };
    private readonly IDisposable appearance;
    private long displayedLogSequence;
    private bool initializing = true;
    private bool changingPage;
    private bool layoutPending;
    private bool closed;
    private bool closeRequested;
    private bool running;
    private bool scanning;
    private bool cancelling;
    private bool inputVerified;
    private int currentStep;
    private int inputKindIndex;
    private string inputPath = "";
    private string outputParent = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "The Simpsons Game PS5");
    private string inputStatus = "Choose your ISO or an extracted gamedata folder.";
    private Brush inputStatusBrush = new SolidColorBrush(Color.FromRgb(168, 168, 168));
    private SettingGroupViewModel? selectedGroup;
    private bool skipIntro;
    private EpisodeOption? selectedEpisode;
    private string? episodeWarning;
    private string? losslessPath;
    private string losslessStatus = "No file selected. Frame generation is off and its controls are unavailable. Choose your file to apply the recommended frame generation setting.";
    private string progressPhase = "Ready to build.";
    private double progressPercent;
    private string progressFile = "";
    private string progressNumbers = "";
    private string outcome = "";
    private Brush outcomeBrush = new SolidColorBrush(Color.FromRgb(168, 168, 168));
    private bool showLog;
    private bool showFullSummary;
    private string logExcerpt = "";
    private CancellationTokenSource? scanCancellation;
    private CancellationTokenSource? buildCancellation;
    private BuildResult? result;

    public event PropertyChangedEventHandler? PropertyChanged;
    private void Changed(params string[] names)
    { foreach (string name in names) PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name)); }

    public ObservableCollection<SettingGroupViewModel> SettingGroups { get; } = [];
    public ObservableCollection<SettingViewModel> LanguageSettings { get; } = [];
    public ObservableCollection<EpisodeOption> Episodes { get; } = [new("Keep the disc / folder starting episode", null)];
    public ObservableCollection<SummaryRow> SummaryRows { get; } = [];
    public ObservableCollection<SummaryRow> FullSummaryRows { get; } = [];
    public IReadOnlyList<string> SourceKinds { get; } = ["Xbox 360 ISO / XISO", "Extracted gamedata folder"];
    public string VersionLabel => $"Builder {Program.BuilderVersion} · PS5 {bundle.Manifest.BuildVersion}";
    public bool IsFilesPage => currentStep == 0;
    public bool IsSettingsPage => currentStep == 1;
    public bool IsLanguagePage => currentStep == 2;
    public bool IsBuildPage => currentStep == 3;
    public string StepLabel => $"STEP {currentStep + 1} OF 4";
    public string Heading => currentStep switch { 0 => "Game files", 1 => "Settings", 2 => "Language", _ => BuildComplete ? "Your PS5 folder is ready" : "Review and build" };
    public string Subtitle => currentStep switch
    {
        0 => "Choose your game files and where to create the PS5 folder.",
        1 => "Choose a category to adjust your PS5 settings.",
        2 => "Select a language included on your game disc.",
        _ => BuildComplete ? "The build is complete and the copied files have been verified." : "Review your choices, then create the folder to transfer to your PS5."
    };
    public bool CanNavigate => !running;
    public bool CanBack => !running && currentStep > 0 && !BuildComplete;
    public bool CanNext => !running && !scanning && !settingModels.Values.Any(s => s.HasError) &&
        (BuildComplete || (currentStep is 1 or 2) || (inputVerified && !string.IsNullOrWhiteSpace(outputParent)));
    public bool CanCancel => !cancelling;
    public string NextLabel => BuildComplete ? "Build another" : currentStep switch { 0 or 1 => "Continue", 2 => "Review build", _ => "Build PS5 folder" };
    public string CancelLabel => running ? "Cancel build" : "Close";
    public string BuildSummaryHeading => BuildComplete ? "Build complete" : "Summary";
    public bool BuildComplete => result is not null;
    public bool HasEpisodes => Episodes.Count > 1;
    public string EpisodeNote => episodeWarning is null ? "Applies to new games. Existing saves keep their progress." : "Episode selection is unavailable: " + episodeWarning;
    public bool LosslessAvailable => losslessPath is not null;
    public string LosslessPathDisplay => losslessPath ?? "No Lossless.dll selected";
    public string LosslessStatus => losslessStatus;
    public string BuildHint => BuildComplete ? "Copy PPSA74207 to /data/homebrew/PPSA74207. Keep userdata when updating. If you use an image, replace the image mounted by ShadowMountPlus." :
        !inputVerified ? "Choose and verify your game source in Game files before building." :
        string.IsNullOrWhiteSpace(outputParent) ? "Choose an output folder in Game files." :
        settingModels.Values.Any(s => s.HasError) ? "Correct the highlighted setting before building." :
        "The builder creates a new PPSA74207 folder. Your original game files remain unchanged.";
    public string SourcePathLabel => inputKindIndex == 0 ? "ISO / XISO path" : "Folder containing default.xex";
    public string InputStatus => inputStatus;
    public Brush InputStatusBrush => inputStatusBrush;
    public string ProgressPhase => progressPhase;
    public double ProgressPercent => progressPercent;
    public string ProgressFile => progressFile;
    public string ProgressNumbers => progressNumbers;
    public string Outcome => outcome;
    public Brush OutcomeBrush => outcomeBrush;
    public bool ShowLog => showLog;
    public string LogExcerpt => logExcerpt;
    public string LogToggleLabel => showLog ? "Hide live log" : "Show live log";
    public bool ShowFullSummary => showFullSummary;
    public string SummaryToggleLabel => showFullSummary ? "Hide all settings" : "Show all settings";

    public int InputKindIndex
    {
        get => inputKindIndex;
        set
        {
            if (value is < 0 or > 1 || value == inputKindIndex) return;
            inputKindIndex = value; InputPath = ""; InvalidateSource();
            Changed(nameof(InputKindIndex), nameof(SourcePathLabel));
        }
    }
    public string InputPath
    {
        get => inputPath;
        set { if (inputPath != value) { inputPath = value; InvalidateSource(); Changed(nameof(InputPath)); } }
    }
    public string OutputParent
    {
        get => outputParent;
        set { if (outputParent != value) { outputParent = value; Changed(nameof(OutputParent)); UpdateNavigation(); } }
    }
    public SettingGroupViewModel? SelectedGroup
    {
        get => selectedGroup;
        set
        {
            if (value is null || selectedGroup == value) return;
            selectedGroup = value; Changed(nameof(SelectedGroup));
            if (!initializing) { diagnostics.Record("ui.category", new { value.Id }); PageScroll?.ScrollToTop(); layoutPending = true; }
        }
    }
    public bool SkipIntro
    {
        get => skipIntro;
        set { if (skipIntro != value) { skipIntro = value; Changed(nameof(SkipIntro)); diagnostics.Record("game-options.skip-intro", new { skipIntro }); } }
    }
    public EpisodeOption? SelectedEpisode
    {
        get => selectedEpisode;
        set { if (selectedEpisode != value) { selectedEpisode = value; Changed(nameof(SelectedEpisode)); if (!initializing) diagnostics.Record("game-options.start-episode", new { startEpisode = value?.Choice?.Id }); } }
    }

    public MainWindow(BundlePackage package, DiagnosticSession session)
    {
        bundle = package; diagnostics = session; settings = bundle.Capabilities.Defaults(losslessAvailable: false);
        foreach (CapabilitySetting setting in bundle.Capabilities.Settings)
            settingModels.Add(setting.Key, new SettingViewModel(setting, settings[setting.Key], SetSetting, OnValidationChanged));
        foreach (CapabilityGroup group in bundle.Capabilities.Groups)
        {
            if (group.Id == "language")
            { foreach (SettingViewModel setting in settingModels.Values.Where(s => s.Definition.Group == group.Id)) LanguageSettings.Add(setting); continue; }
            // Keep the native audio_mute=false value in the generated TOML,
            // while leaving the requested builder interface without a mute control.
            SettingGroups.Add(new SettingGroupViewModel(group, settingModels.Values.Where(s => s.Definition.Group == group.Id && s.Key != "audio_mute"), bundle.Capabilities.Presets.Where(p => p.Group == group.Id), ApplyPreset));
        }
        selectedGroup = SettingGroups[0]; selectedEpisode = Episodes[0];
        InitializeComponent(); DataContext = this;
        appearance = WindowAppearance.Attach(this, diagnostics);
        logRefresh.Tick += LogRefresh_Tick; logRefresh.Start();
        initializing = false; RefreshDependencies(); UpdateNavigation();
        ContentRendered += (_, _) => { diagnostics.Record("ui.shown", new { framework = "WPF", version = Program.BuilderVersion }); layoutPending = true; };
    }

    private void InvalidateSource()
    {
        scanCancellation?.Cancel(); inputVerified = false; result = null;
        inputStatus = string.IsNullOrWhiteSpace(inputPath) ? "Choose your ISO or an extracted gamedata folder." : "The source will be checked when you leave this field.";
        inputStatusBrush = BrushFor("TextSecondary");
        Episodes.Clear(); Episodes.Add(new("Keep the disc / folder starting episode", null)); selectedEpisode = Episodes[0]; episodeWarning = null;
        Changed(nameof(InputStatus), nameof(InputStatusBrush), nameof(HasEpisodes), nameof(SelectedEpisode), nameof(EpisodeNote), nameof(BuildComplete)); UpdateNavigation();
    }

    private async void BrowseSource_Click(object sender, RoutedEventArgs e)
    {
        string? chosen = null;
        if (inputKindIndex == 0)
        {
            var dialog = new OpenFileDialog { Title = "Choose The Simpsons Game Xbox 360 ISO", Filter = "Xbox 360 disc images (*.iso;*.xiso)|*.iso;*.xiso|All files (*.*)|*.*", CheckFileExists = true, Multiselect = false };
            if (dialog.ShowDialog(this) == true) chosen = dialog.FileName;
        }
        else
        {
            var dialog = new OpenFolderDialog { Title = "Choose the folder containing default.xex", Multiselect = false };
            if (dialog.ShowDialog(this) == true) chosen = dialog.FolderName;
        }
        if (chosen is null) return;
        InputPath = chosen; await InspectInputAsync();
    }
    private void BrowseOutput_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFolderDialog { Title = "Choose the parent folder for PPSA74207", Multiselect = false };
        if (Directory.Exists(outputParent)) dialog.InitialDirectory = outputParent;
        if (dialog.ShowDialog(this) == true) OutputParent = dialog.FolderName;
    }
    private async void SourcePath_LostFocus(object sender, KeyboardFocusChangedEventArgs e)
    { if (!inputVerified && !string.IsNullOrWhiteSpace(inputPath) && !scanning && !closed) await InspectInputAsync(); }

    private async Task InspectInputAsync()
    {
        scanCancellation?.Cancel(); scanCancellation?.Dispose();
        var cancellation = new CancellationTokenSource(); scanCancellation = cancellation;
        BuildInput selected = new((InputKind)inputKindIndex, inputPath.Trim());
        if (string.IsNullOrWhiteSpace(selected.Path)) return;
        var clock = Stopwatch.StartNew(); diagnostics.Record("inspection.start", selected);
        scanning = true; inputVerified = false; SetInputStatus("Checking game files…", "TextSecondary"); UpdateNavigation();
        try
        {
            var inspection = await Task.Run(() =>
            {
                using GameSource source = GameSource.Open(selected, bundle.Manifest.XboxTitleId, cancellation.Token, log: diagnostics.Write);
                IReadOnlyList<EpisodeChoice> available = []; string? warning = null;
                try { available = GamePatches.ReadEpisodes(source, cancellation.Token); }
                catch (BuilderException error) { warning = error.Message; diagnostics.Error("inspection.episodes-unavailable", error); }
                return (source.FileCount, source.TotalBytes, Episodes: available, Warning: warning);
            }, cancellation.Token);
            if (closed || cancellation != scanCancellation || selected.Path != inputPath.Trim() || selected.Kind != (InputKind)inputKindIndex) return;
            inputVerified = true; episodeWarning = inspection.Warning;
            string? previous = selectedEpisode?.Choice?.Id;
            Episodes.Clear(); Episodes.Add(new("Keep the disc / folder starting episode", null));
            foreach (EpisodeChoice episode in inspection.Episodes) Episodes.Add(new(episode.Label, episode));
            selectedEpisode = Episodes.FirstOrDefault(ep => ep.Choice?.Id == previous) ?? Episodes[0];
            Changed(nameof(SelectedEpisode), nameof(HasEpisodes), nameof(EpisodeNote));
            SetInputStatus($"The Simpsons Game confirmed · {inspection.FileCount:N0} files · {Format.Bytes(inspection.TotalBytes)}", "Good");
            diagnostics.Record("inspection.complete", new { inspection.FileCount, inspection.TotalBytes, episodes = inspection.Episodes.Select(ep => ep.Id), warning = episodeWarning, durationMs = clock.ElapsedMilliseconds });
        }
        catch (OperationCanceledException) { diagnostics.Record("inspection.cancelled", new { durationMs = clock.ElapsedMilliseconds }); }
        catch (Exception error)
        {
            diagnostics.Error("inspection.failed", error, new { selected.Path, durationMs = clock.ElapsedMilliseconds });
            if (!closed && cancellation == scanCancellation && selected.Path == inputPath.Trim() && selected.Kind == (InputKind)inputKindIndex)
                SetInputStatus(error is BuilderException ? error.Message : "The game files could not be read. " + error.Message, "Error");
        }
        finally { if (!closed && cancellation == scanCancellation) { scanning = false; UpdateNavigation(); } }
    }
    private void SetInputStatus(string value, string brush)
    { inputStatus = value; inputStatusBrush = BrushFor(brush); Changed(nameof(InputStatus), nameof(InputStatusBrush)); }

    private void SetSetting(string key, JsonElement value)
    { settings[key] = value.Clone(); diagnostics.Record("settings.change", new { key, value }); RefreshDependencies(); }
    private void OnValidationChanged() { if (!initializing) UpdateNavigation(); }
    private void ApplyPreset(CapabilityPreset preset)
    {
        foreach ((string key, JsonElement value) in preset.Values) { settings[key] = value.Clone(); settingModels[key].Synchronize(value); }
        diagnostics.Record("settings.preset", new { preset.Id, preset.Values }); RefreshDependencies();
    }
    private void RestoreRecommended_Click(object sender, RoutedEventArgs e)
    {
        Dictionary<string, JsonElement> recommended = bundle.Capabilities.Defaults(LosslessAvailable);
        foreach (CapabilitySetting definition in bundle.Capabilities.Settings)
        { settings[definition.Key] = recommended[definition.Key]; settingModels[definition.Key].Synchronize(recommended[definition.Key]); }
        SkipIntro = false; SelectedEpisode = Episodes[0];
        diagnostics.Record("settings.restore-recommended", settings); RefreshDependencies();
    }
    private void RefreshDependencies()
    {
        foreach (SettingViewModel model in settingModels.Values)
        {
            CapabilitySetting setting = model.Definition;
            bool needsLibrary = bundle.Capabilities.Groups.Any(g => g.Id == setting.Group && g.RequiresFile == "lossless.dll");
            model.IsEnabled = (!needsLibrary || LosslessAvailable) && bundle.Capabilities.IsEnabled(setting, settings) && setting.ApplyMode != "fixed";
            model.IsExperimental = setting.Status == "experimental" || setting.Choices.Any(c => c.Status == "experimental" && Capabilities.Equal(c.Value, settings[setting.Key]));
        }
        foreach (SettingGroupViewModel group in SettingGroups) group.RefreshPreset(settings);
        UpdateNavigation();
    }
    private void ChooseLossless_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFileDialog { Title = "Choose your Lossless.dll", Filter = "Lossless DLL (Lossless.dll)|Lossless.dll|DLL files (*.dll)|*.dll|All files (*.*)|*.*", CheckFileExists = true, Multiselect = false };
        if (dialog.ShowDialog(this) != true) return;
        try
        {
            using OptionalLibrary selected = OptionalLibrary.Open(dialog.FileName);
            bool wasAvailable = LosslessAvailable;
            losslessPath = selected.FullPath; losslessStatus = "File selected. Frame generation controls are available.";
            bundle.Capabilities.ApplyOptionalFileAvailability(settings, wasAvailable, isAvailable: true);
            SynchronizeOptionalSettings();
            diagnostics.Record("optional-library.selected", new { selected.FullPath, selected.Length, validation = "ordinary file access only", appliedRecommendedSwitches = !wasAvailable, frameGeneration = settings["frame_gen"] });
            RefreshDependencies();
        }
        catch (Exception error) { diagnostics.Error("optional-library.selection-failed", error, new { path = dialog.FileName }); losslessStatus = "The file could not be opened. " + error.Message; }
        Changed(nameof(LosslessAvailable), nameof(LosslessPathDisplay), nameof(LosslessStatus));
    }
    private void RemoveLossless_Click(object sender, RoutedEventArgs e)
    {
        bool wasAvailable = LosslessAvailable;
        losslessPath = null; losslessStatus = "No file selected. Frame generation is off and its controls are unavailable. Choose your file to apply the recommended frame generation setting.";
        bundle.Capabilities.ApplyOptionalFileAvailability(settings, wasAvailable, isAvailable: false);
        SynchronizeOptionalSettings();
        diagnostics.Record("optional-library.removed", new { frameGeneration = settings["frame_gen"], otherFrameGenerationOptions = "restored defaults" });
        Changed(nameof(LosslessAvailable), nameof(LosslessPathDisplay), nameof(LosslessStatus)); RefreshDependencies();
    }
    private void SynchronizeOptionalSettings()
    {
        foreach (SettingViewModel model in settingModels.Values.Where(s => bundle.Capabilities.Groups.Any(g => g.Id == s.Definition.Group && g.RequiresFile == "lossless.dll")))
            model.Synchronize(settings[model.Key]);
    }
    private void ChooseColour_Click(object sender, RoutedEventArgs e)
    {
        if (sender is not FrameworkElement { DataContext: SettingViewModel model }) return;
        var dialog = new ColourWindow(((SolidColorBrush)model.ColorBrush).Color.ToString(), diagnostics) { Owner = this };
        if (dialog.ShowDialog() == true) model.ColorHex = dialog.SelectedHex;
    }

    internal void ShowPage(int index)
    {
        if (index is < 0 or > 3 || (running && index != 3) || changingPage) return;
        changingPage = true; currentStep = index;
        try
        {
            if (index == 3) RefreshSummary();
            Changed(nameof(IsFilesPage), nameof(IsSettingsPage), nameof(IsLanguagePage), nameof(IsBuildPage), nameof(StepLabel), nameof(Heading), nameof(Subtitle));
            UpdateNavigation(); PageScroll.ScrollToTop(); layoutPending = true;
            diagnostics.Record("ui.step", new { step = currentStep + 1 });
        }
        finally { changingPage = false; }
    }
    private void Navigation_Checked(object sender, RoutedEventArgs e)
    {
        if (!initializing && !changingPage && sender is FrameworkElement { Tag: string text } && int.TryParse(text, out int index) && index != currentStep) ShowPage(index);
    }
    private void Back_Click(object sender, RoutedEventArgs e) { if (CanBack) ShowPage(currentStep - 1); }
    private async void Next_Click(object sender, RoutedEventArgs e)
    {
        if (!CanNext) return;
        if (BuildComplete)
        { result = null; outcome = ""; progressPercent = 0; progressPhase = "Ready to build."; progressFile = progressNumbers = ""; UpdateProgress(); ShowPage(0); return; }
        if (currentStep < 3) ShowPage(currentStep + 1); else await StartBuildAsync();
    }
    private void Cancel_Click(object sender, RoutedEventArgs e) { if (running) RequestCancellation(); else Close(); }
    private void RequestCancellation()
    {
        if (cancelling) return;
        cancelling = true; diagnostics.Record("ui.cancel-requested"); buildCancellation?.Cancel();
        progressPhase = "Cancelling safely…"; Changed(nameof(ProgressPhase)); UpdateNavigation();
    }
    private void UpdateNavigation()
    {
        Changed(nameof(CanNavigate), nameof(CanBack), nameof(CanNext), nameof(CanCancel), nameof(NextLabel), nameof(CancelLabel),
            nameof(BuildComplete), nameof(BuildHint), nameof(BuildSummaryHeading), nameof(Heading), nameof(Subtitle));
    }
    private void RefreshSummary()
    {
        SummaryRows.Clear(); FullSummaryRows.Clear();
        SummaryRows.Add(new("Game source", string.IsNullOrWhiteSpace(inputPath) ? "Not selected" : inputPath));
        SummaryRows.Add(new("Output folder", result?.OutputDirectory ?? (string.IsNullOrWhiteSpace(outputParent) ? "Not selected" : Path.Combine(outputParent.Trim(), "PPSA74207"))));
        foreach (string key in new[] { "resolution_scale", "frame_rate", "prospero_output_hz", "user_language", "frame_gen" })
            if (settingModels.TryGetValue(key, out SettingViewModel? setting)) SummaryRows.Add(new(setting.Label, setting.CurrentLabel));
        foreach (SettingViewModel model in settingModels.Values.Where(s => s.Key != "audio_mute")) FullSummaryRows.Add(new(model.Label, model.CurrentLabel));
        FullSummaryRows.Add(new("Lossless.dll", losslessPath ?? "Not included"));
        FullSummaryRows.Add(new("Skip intro logos", skipIntro ? "On" : "Off"));
        FullSummaryRows.Add(new("Starting episode", selectedEpisode?.Label ?? "Unchanged"));
        if (result is not null) SummaryRows.Add(new("Verified game files", $"{result.FileCount:N0} · {Format.Bytes(result.GameBytes)} · {result.Duration.TotalMinutes:0.0} minutes"));
    }
    private async Task StartBuildAsync()
    {
        if (running || !inputVerified || string.IsNullOrWhiteSpace(outputParent) || settingModels.Values.Any(s => s.HasError)) return;
        result = null; running = true; cancelling = false; outcome = ""; progressPercent = 0; progressPhase = "Preparing…"; progressFile = progressNumbers = "";
        buildCancellation = new CancellationTokenSource();
        diagnostics.Record("ui.build-requested", new { buildId = Guid.NewGuid().ToString("N"), inputKind = (InputKind)inputKindIndex, inputPath, outputParent });
        UpdateProgress(); ShowPage(3);
        try
        {
            long lastProgressLog = 0; string lastProgressPhase = ""; var clock = Stopwatch.StartNew();
            var progress = new Progress<BuildProgress>(p =>
            {
                if (closed || !running) return;
                if (p.Phase != lastProgressPhase || clock.ElapsedMilliseconds - lastProgressLog >= 1000)
                { lastProgressLog = clock.ElapsedMilliseconds; lastProgressPhase = p.Phase; diagnostics.Record("ui.progress", p); }
                progressPercent = Math.Clamp(p.Percent, 0, 100); progressPhase = cancelling ? "Cancelling safely…" : p.Message; progressFile = p.CurrentFile ?? "";
                progressNumbers = p.BytesTotal > 0 ? $"{Format.Bytes(p.BytesDone)} / {Format.Bytes(p.BytesTotal)} · {p.FilesDone:N0} / {p.FilesTotal:N0} files" : "";
                UpdateProgress();
            });
            var request = new BuildRequest(new((InputKind)inputKindIndex, inputPath.Trim()), outputParent.Trim(),
                settings.ToDictionary(p => p.Key, p => p.Value.Clone()), new GamePatchOptions(skipIntro, selectedEpisode?.Choice?.Id), losslessPath);
            result = await new BuildPipeline(bundle).BuildAsync(request, progress, diagnostics.Write, buildCancellation.Token);
            diagnostics.Record("ui.build-complete", result); await diagnostics.FlushAsync();
            running = false; progressPercent = 100; progressPhase = "Build complete."; progressFile = "";
            outcome = "Your PS5 folder is ready."; outcomeBrush = BrushFor("Good"); ShowPage(3);
        }
        catch (OperationCanceledException)
        {
            diagnostics.Record("ui.build-cancelled"); await diagnostics.FlushAsync(); progressPhase = "Build cancelled.";
            outcome = "Your original files and existing output folders have been kept."; outcomeBrush = BrushFor("TextSecondary");
        }
        catch (Exception error)
        {
            diagnostics.Error("ui.build-failed", error); await diagnostics.FlushAsync(); progressPhase = "The build could not be completed.";
            outcome = error.Message; outcomeBrush = BrushFor("Error"); ShowError("Build could not be completed", error);
        }
        finally
        {
            running = false; cancelling = false; buildCancellation.Dispose(); buildCancellation = null;
            UpdateProgress(); UpdateNavigation(); RefreshLogPreview(); if (closeRequested) Close();
        }
    }
    private void UpdateProgress() => Changed(nameof(ProgressPhase), nameof(ProgressPercent), nameof(ProgressFile), nameof(ProgressNumbers), nameof(Outcome), nameof(OutcomeBrush));

    private void ToggleSummary_Click(object sender, RoutedEventArgs e) { showFullSummary = !showFullSummary; Changed(nameof(ShowFullSummary), nameof(SummaryToggleLabel)); }
    private void ToggleLog_Click(object sender, RoutedEventArgs e) { showLog = !showLog; Changed(nameof(ShowLog), nameof(LogToggleLabel)); displayedLogSequence = -1; RefreshLogPreview(); }
    private void OpenOutput_Click(object sender, RoutedEventArgs e) { if (result is not null) OpenPath(result.OutputDirectory); }
    private void OpenLogs_Click(object sender, RoutedEventArgs e) => OpenLogs();
    private void OpenLogs()
    {
        if (diagnostics.LogPath is not null) OpenPath(Path.GetDirectoryName(diagnostics.LogPath)!);
        else MessageBox.Show(this, diagnostics.LocationDescription + "\n\n" + diagnostics.StorageProblem, "Diagnostics", MessageBoxButton.OK, MessageBoxImage.Information);
    }
    private async void CopyDiagnostics_Click(object sender, RoutedEventArgs e) => await CopyDiagnosticsAsync();
    private async Task CopyDiagnosticsAsync()
    {
        try { Clipboard.SetText(await diagnostics.DiagnosticTextAsync()); MessageBox.Show(this, "Diagnostics copied to the clipboard.", "Diagnostics", MessageBoxButton.OK, MessageBoxImage.Information); }
        catch (Exception error) { diagnostics.Error("diagnostics.copy-failed", error); ShowError("Diagnostics could not be copied", error); }
    }
    private async void ExportLog_Click(object sender, RoutedEventArgs e) => await ExportLogAsync();
    private async Task ExportLogAsync()
    {
        var dialog = new SaveFileDialog { Title = "Export builder diagnostics", Filter = "Diagnostic log (*.log)|*.log", OverwritePrompt = false,
            FileName = "Simpsons-Builder-" + DateTimeOffset.UtcNow.ToString("yyyyMMdd-HHmmss", CultureInfo.InvariantCulture) + ".log" };
        if (dialog.ShowDialog(this) != true) return;
        try
        {
            await diagnostics.ExportAsync(dialog.FileName); diagnostics.Record("diagnostics.exported", new { path = dialog.FileName });
            MessageBox.Show(this, "Diagnostics exported to:\n" + dialog.FileName, "Diagnostics", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        catch (Exception error) { diagnostics.Error("diagnostics.export-failed", error); ShowError("Diagnostics could not be exported. Choose a new file name", error); }
    }
    private void About_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new AboutWindow(VersionLabel, diagnostics, Program.Notices(), OpenLogs, CopyDiagnosticsAsync, ExportLogAsync) { Owner = this };
        dialog.ShowDialog();
    }
    private void Donate_Click(object sender, RoutedEventArgs e) => ProjectLinks.OpenDonation(this, diagnostics);
    private void LogRefresh_Tick(object? sender, EventArgs e)
    {
        RefreshLogPreview();
        if (layoutPending && IsLoaded) { layoutPending = false; RecordLayout(); }
    }
    private void RefreshLogPreview()
    {
        if (!showLog || displayedLogSequence == diagnostics.LastSequence || closed) return;
        displayedLogSequence = diagnostics.LastSequence; logExcerpt = diagnostics.RecentText(80); Changed(nameof(LogExcerpt)); LiveLogBox.ScrollToEnd();
    }
    private void OpenPath(string path)
    {
        try { Process.Start(new ProcessStartInfo(path) { UseShellExecute = true }); }
        catch (Exception error) { diagnostics.Error("ui.open-path-failed", error, new { path }); ShowError("The file or folder could not be opened", error); }
    }
    private void ShowError(string title, Exception error) => MessageBox.Show(this, error.Message + "\n\nDiagnostics:\n" + diagnostics.LocationDescription, title, MessageBoxButton.OK, MessageBoxImage.Error);
    private Brush BrushFor(string key) => TryFindResource(key) as Brush ?? new SolidColorBrush(Color.FromRgb(168, 168, 168));

    private void Minimize_Click(object sender, RoutedEventArgs e) => SystemCommands.MinimizeWindow(this);
    private void Maximize_Click(object sender, RoutedEventArgs e)
    { if (WindowState == WindowState.Maximized) SystemCommands.RestoreWindow(this); else SystemCommands.MaximizeWindow(this); }
    private void Close_Click(object sender, RoutedEventArgs e) => Close();
    private void Window_SizeChanged(object sender, SizeChangedEventArgs e)
    {
        if (SidebarColumn is null || MainArea is null) return;
        bool compact = ActualWidth < 800;
        Sidebar.Visibility = compact ? Visibility.Collapsed : Visibility.Visible;
        CompactAboutButton.Visibility = compact ? Visibility.Visible : Visibility.Collapsed;
        SidebarColumn.Width = new GridLength(compact ? 0 : ActualWidth < 1040 ? 190 : 230);
        MainArea.Margin = ActualWidth < 1040 ? new Thickness(22, 20, 20, 18) : new Thickness(32, 22, 32, 20);
        layoutPending = true;
    }
    private void Window_StateChanged(object? sender, EventArgs e)
    {
        if (MaximizeButton is not null) { MaximizeButton.Content = WindowState == WindowState.Maximized ? "\uE923" : "\uE922"; MaximizeButton.ToolTip = WindowState == WindowState.Maximized ? "Restore" : "Maximize"; }
        layoutPending = true;
    }
    private void Window_Closing(object? sender, CancelEventArgs e)
    {
        diagnostics.Record("ui.close-requested", new { running, scanning }); scanCancellation?.Cancel();
        if (!running) return;
        e.Cancel = true; closeRequested = true; RequestCancellation();
    }
    private void Window_Closed(object? sender, EventArgs e)
    {
        closed = true; logRefresh.Stop(); logRefresh.Tick -= LogRefresh_Tick;
        scanCancellation?.Cancel(); scanCancellation?.Dispose(); buildCancellation?.Dispose(); appearance.Dispose();
    }
    private void RecordLayout()
    {
        DpiScale dpi = VisualTreeHelper.GetDpi(this);
        diagnostics.Record("ui.layout", new { page = currentStep + 1, category = selectedGroup?.Id, state = WindowState.ToString(),
            width = ActualWidth, height = ActualHeight, dpiX = dpi.PixelsPerInchX, dpiY = dpi.PixelsPerInchY,
            sidebarWidth = SidebarColumn.ActualWidth, contentWidth = PageScroll.ViewportWidth, contentHeight = PageScroll.ViewportHeight,
            extentHeight = PageScroll.ExtentHeight, actionBarWidth = ActionBar.ActualWidth, nextButtonWidth = NextButton.ActualWidth,
            nextButtonHeight = NextButton.ActualHeight, sourceFieldWidth = SourcePathBox.ActualWidth, sourceFieldHeight = SourcePathBox.ActualHeight });
    }

    internal async Task SavePreviewAsync(string directory)
    {
        Directory.CreateDirectory(directory);
        var captures = new List<object>();
        for (int index = 0; index < 4; index++)
        {
            ShowPage(index);
            await Dispatcher.InvokeAsync(() => UpdateLayout(), DispatcherPriority.ApplicationIdle);
            DpiScale dpi = VisualTreeHelper.GetDpi(this);
            int width = (int)Math.Ceiling(ActualWidth * dpi.DpiScaleX), height = (int)Math.Ceiling(ActualHeight * dpi.DpiScaleY);
            var bitmap = new RenderTargetBitmap(width, height, dpi.PixelsPerInchX, dpi.PixelsPerInchY, PixelFormats.Pbgra32); bitmap.Render(this);
            var encoder = new PngBitmapEncoder(); encoder.Frames.Add(BitmapFrame.Create(bitmap));
            using FileStream file = File.Create(Path.Combine(directory, $"step-{index + 1}.png")); encoder.Save(file);
            captures.Add(new { page = index + 1, width, height, contentWidth = PageScroll.ViewportWidth, actionBarHeight = ActionBar.ActualHeight });
        }
        File.WriteAllText(Path.Combine(directory, "preview.json"), JsonSerializer.Serialize(new { status = "CAPTURED", framework = "WPF", captures,
            note = "Actual runtime captures; capture completion alone is not a clipping or visual QA verdict." }, JsonFiles.Options));
    }
}
