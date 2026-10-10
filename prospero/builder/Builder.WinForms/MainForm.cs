using System.Diagnostics;
using System.Drawing.Imaging;
using System.Text;
using System.Text.Json;

namespace Simpsons.Builder.Desktop;

internal sealed class MainForm : Form
{
    private readonly BundlePackage bundle;
    private readonly DiagnosticSession diagnostics;
    private readonly System.Windows.Forms.Timer logRefresh = new() { Interval = 500 };
    private long displayedLogSequence;
    private Dictionary<string, JsonElement> settings;
    private readonly FlowLayoutPanel content;
    private readonly Label heading;
    private readonly Label subtitle;
    private readonly Label stepLabel;
    private readonly ActionButton next = new("Next: Graphics  →", true) { Width = 205 };
    private readonly ActionButton back = new("Back", false) { Width = 105 };
    private readonly ActionButton cancel = new("Close", false) { Width = 105 };
    private readonly StepButton[] steps;
    private readonly Dictionary<string, Control> editors = [];
    private readonly Dictionary<string, Action<JsonElement>> editorWriters = [];
    private readonly Dictionary<string, Label> experimentalLabels = [];
    private readonly List<(string Group, ComboBox Combo)> presetCombos = [];
    private readonly Font titleFont;
    private readonly Image cover;
    private int currentStep;
    private InputKind inputKind;
    private string inputPath = "";
    private string outputParent = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "The Simpsons Game PS5");
    private Label? inputStatus;
    private bool inputVerified;
    private string verifiedDescription = "";
    private bool scanning;
    private bool running;
    private bool syncing;
    private bool closeRequested;
    private bool skipIntro;
    private string? startEpisode;
    private string? losslessPath;
    private bool losslessAvailable;
    private IReadOnlyList<EpisodeChoice> episodes = [];
    private string? episodeWarning;
    private CancellationTokenSource? scanCancellation;
    private CancellationTokenSource? buildCancellation;
    private BuildResult? result;
    private ProgressBar? progressBar;
    private Label? progressPhase;
    private Label? progressFile;
    private Label? progressNumbers;
    private Label? outcomeLabel;
    private TextBox? buildDetails;
    public MainForm(BundlePackage package, DiagnosticSession session)
    {
        bundle = package;
        diagnostics = session;
        settings = bundle.Capabilities.Defaults();
        Text = "The Simpsons Game · PS5 Builder";
        ClientSize = new Size(1170, 830);
        MinimumSize = new Size(1050, 730);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Theme.Page;
        Font = Theme.Body();
        AutoScaleMode = AutoScaleMode.Dpi;
        AutoScaleDimensions = new SizeF(96, 96);
        titleFont = Theme.Title(26);
        using (Stream stream = Program.Resource("cover.png")) cover = new Bitmap(Image.FromStream(stream));
        Icon = CreateAppIcon(cover);

        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 1, Margin = Padding.Empty };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 262));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        Controls.Add(layout);

        var sidebar = new SkyPanel { Dock = DockStyle.Fill, AutoScroll = true, Padding = new Padding(18, 15, 18, 18), Margin = Padding.Empty };
        layout.Controls.Add(sidebar, 0, 0);
        var brand = new Panel { Dock = DockStyle.Top, Height = 248, BackColor = Color.Transparent };
        var picture = new PictureBox { Image = cover, SizeMode = PictureBoxSizeMode.Zoom, Size = new Size(102, 102), Location = new Point(61, 4), BackColor = Color.Transparent };
        brand.Controls.Add(picture);
        var name = new Label { Text = "THE SIMPSONS\nGAME", Font = Theme.Title(25), ForeColor = Theme.Yellow, TextAlign = ContentAlignment.MiddleCenter,
            Location = new Point(0, 115), Size = new Size(226, 79), BackColor = Color.Transparent };
        brand.Controls.Add(name);
        var builderLabel = Theme.Label("PS5 BUILDER", 10, true, Color.White);
        builderLabel.AutoSize = false; builderLabel.TextAlign = ContentAlignment.MiddleCenter;
        builderLabel.SetBounds(0, 199, 226, 29); brand.Controls.Add(builderLabel);

        var stepPanel = new FlowLayoutPanel { Dock = DockStyle.Top, Height = 334, FlowDirection = FlowDirection.TopDown, WrapContents = false, BackColor = Color.Transparent };
        steps = [new(1, "Game files", "Your disc. Your adventure."), new(2, "Graphics & more", "Make it look your way."),
                 new(3, "Language", "Springfield, in your language."), new(4, "Build", "One folder. Ready for PS5.")];
        for (int i = 0; i < steps.Length; i++)
        {
            int index = i;
            steps[i].Click += (_, _) => { if (!running && !scanning && (index == 0 || inputVerified)) ShowStep(index); };
            stepPanel.Controls.Add(steps[i]);
        }
        var footer = new Panel { Dock = DockStyle.Bottom, Height = 95, BackColor = Color.Transparent };
        var local = Theme.Label("YOUR FILES STAY LOCAL", 9, true, Color.White);
        local.Location = new Point(9, 8); footer.Controls.Add(local);
        var version = Theme.Label($"Builder 1.0  ·  PS5 {bundle.Manifest.BuildVersion}", 9, false, Color.FromArgb(222, 244, 255));
        version.Location = new Point(9, 33); footer.Controls.Add(version);
        var about = new LinkLabel { Text = "About & credits", AutoSize = true, Font = Theme.Body(9), BackColor = Color.Transparent,
            LinkColor = Color.White, ActiveLinkColor = Theme.Yellow, Location = new Point(9, 60) };
        about.LinkClicked += (_, _) => ShowCredits(); footer.Controls.Add(about);
        var logs = new LinkLabel { Text = "Open logs", AutoSize = true, Font = Theme.Body(9), BackColor = Color.Transparent,
            LinkColor = Color.White, ActiveLinkColor = Theme.Yellow, Location = new Point(133, 60) };
        logs.LinkClicked += (_, _) => OpenLogs(); footer.Controls.Add(logs);
        sidebar.Controls.Add(stepPanel); sidebar.Controls.Add(brand); sidebar.Controls.Add(footer);
        sidebar.SizeChanged += (_, _) =>
        {
            int needed = brand.Height + stepPanel.Height + footer.Height + sidebar.Padding.Vertical;
            if (sidebar.AutoScrollMinSize.Height != needed) sidebar.AutoScrollMinSize = new Size(0, needed);
        };

        var main = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 3, Margin = Padding.Empty };
        main.RowStyles.Add(new RowStyle(SizeType.Absolute, 138));
        main.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        main.RowStyles.Add(new RowStyle(SizeType.Absolute, 82));
        layout.Controls.Add(main, 1, 0);
        var header = new Panel { Dock = DockStyle.Fill, Padding = new Padding(32, 20, 28, 0) };
        stepLabel = Theme.Label("STEP 1 OF 4", 9, true, Theme.Sky); stepLabel.Location = new Point(33, 21);
        heading = new Label { Font = titleFont, ForeColor = Theme.Ink, AutoSize = false, Location = new Point(29, 45), Height = 45, Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right };
        subtitle = Theme.Label("", 10, false, Theme.Muted); subtitle.AutoSize = false;
        subtitle.SetBounds(33, 93, 730, 42); subtitle.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
        header.Controls.AddRange([stepLabel, heading, subtitle]);
        var compactAbout = new LinkLabel { Text = "About & logs", AutoSize = true, Font = Theme.Body(9), LinkColor = Theme.Sky, Visible = false };
        compactAbout.LinkClicked += (_, _) => ShowCredits(); header.Controls.Add(compactAbout);
        header.Resize += (_, _) =>
        {
            heading.Width = Math.Max(1, header.ClientSize.Width - Dpi(56));
            subtitle.Width = Math.Max(1, header.ClientSize.Width - Dpi(66));
            compactAbout.Location = new Point(Math.Max(Dpi(33), header.ClientSize.Width - compactAbout.Width - Dpi(30)), Dpi(21));
        };
        main.Controls.Add(header, 0, 0);
        content = new FlowLayoutPanel { Dock = DockStyle.Fill, AutoScroll = true, FlowDirection = FlowDirection.TopDown,
            WrapContents = false, Padding = new Padding(32, 5, 27, 10), Margin = Padding.Empty, BackColor = Theme.Page };
        content.SizeChanged += (_, _) => FitCards();
        main.Controls.Add(content, 0, 1);
        var navigation = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(26, 13, 23, 11), ColumnCount = 3 };
        navigation.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 120));
        navigation.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        navigation.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 350));
        navigation.Controls.Add(back, 0, 0);
        var rightNav = new FlowLayoutPanel { FlowDirection = FlowDirection.RightToLeft, Dock = DockStyle.Fill, WrapContents = false };
        rightNav.Controls.Add(next); rightNav.Controls.Add(cancel); navigation.Controls.Add(rightNav, 2, 0);
        main.Controls.Add(navigation, 0, 2);
        void FitShell()
        {
            bool showSidebar = ClientSize.Width >= Dpi(850);
            sidebar.Visible = showSidebar; compactAbout.Visible = !showSidebar;
            layout.ColumnStyles[0].Width = showSidebar ? Dpi(262) : 0;
        }
        ClientSizeChanged += (_, _) => FitShell();
        Shown += (_, _) => { FitWindowToWorkingArea(); FitShell(); diagnostics.Record("ui.shown", new { DeviceDpi, ClientSize, workingArea = Screen.FromHandle(Handle).WorkingArea }); };
        DpiChanged += (_, e) => { diagnostics.Record("ui.dpi-changed", new { e.DeviceDpiOld, e.DeviceDpiNew }); BeginInvoke((Action)(() => { FitWindowToWorkingArea(); FitShell(); })); };
        back.Click += (_, _) => { if (currentStep > 0 && !running) ShowStep(currentStep - 1); };
        next.Click += async (_, _) =>
        {
            if (running) return;
            if (result is not null) { result = null; ShowStep(0); return; }
            if (currentStep < 3) ShowStep(currentStep + 1);
            else await StartBuild();
        };
        cancel.Click += (_, _) =>
        {
            if (running) { diagnostics.Record("ui.cancel-requested"); buildCancellation?.Cancel(); cancel.Enabled = false; if (progressPhase is not null) progressPhase.Text = "Cancelling safely…"; }
            else Close();
        };
        FormClosing += (_, e) =>
        {
            diagnostics.Record("ui.close-requested", new { running, scanning });
            scanCancellation?.Cancel();
            if (!running) return;
            e.Cancel = true; closeRequested = true; buildCancellation?.Cancel();
            if (progressPhase is not null) progressPhase.Text = "Stopping the build before closing…";
        };
        logRefresh.Tick += (_, _) => RefreshLogPreview(); logRefresh.Start();
        ShowStep(0);
    }

    private void ShowStep(int index)
    {
        diagnostics.Record("ui.step", new { step = index + 1 });
        currentStep = index;
        content.SuspendLayout();
        while (content.Controls.Count > 0) content.Controls[0].Dispose();
        editors.Clear(); editorWriters.Clear(); experimentalLabels.Clear(); presetCombos.Clear();
        inputStatus = null; progressBar = null; progressPhase = null; progressFile = null; progressNumbers = null; buildDetails = null; outcomeLabel = null;
        stepLabel.Text = $"STEP {index + 1} OF 4";
        for (int i = 0; i < steps.Length; i++) { steps[i].Selected = i == index; steps[i].Done = i < index || result is not null; steps[i].Invalidate(); }
        switch (index)
        {
            case 0: GameFilesPage(); break;
            case 1: GraphicsPage(); break;
            case 2: LanguagePage(); break;
            case 3: BuildPage(); break;
        }
        foreach (Card card in content.Controls.OfType<Card>()) { card.ResumeLayout(true); card.PerformAutoScale(); }
        content.ResumeLayout(true);
        FitCards(); UpdateNavigation();
        content.AutoScrollPosition = Point.Empty;
    }

    private void GameFilesPage()
    {
        heading.Text = "YOUR GAME. READY FOR PS5.";
        subtitle.Text = "Start with your Xbox 360 disc image. Extraction and verification happen locally.";
        TableLayoutPanel files = AddCard("1. Choose your game files", "Use your Xbox 360 copy of The Simpsons Game.");
        var kind = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Width = 310, Font = Theme.Body(), Margin = new Padding(0, 4, 0, 12) };
        kind.Items.AddRange(["Xbox 360 ISO / XISO", "Already extracted game data folder"]); kind.SelectedIndex = (int)inputKind;
        kind.SelectedIndexChanged += (_, _) =>
        {
            if (inputKind == (InputKind)kind.SelectedIndex) return;
            inputKind = (InputKind)kind.SelectedIndex; inputPath = ""; inputVerified = false; verifiedDescription = ""; episodes = []; startEpisode = null;
            scanCancellation?.Cancel(); ShowStep(0);
        };
        files.Controls.Add(kind);
        var sourceRow = new TableLayoutPanel { ColumnCount = 2, AutoSize = true, Dock = DockStyle.Top, Margin = Padding.Empty };
        sourceRow.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100)); sourceRow.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 130));
        var sourceText = new TextBox { Text = inputPath, Dock = DockStyle.Fill, Font = Theme.Body(10), Margin = new Padding(0, 10, 8, 0), PlaceholderText = inputKind == InputKind.Iso ? "Choose your game ISO…" : "Choose the folder containing default.xex…" };
        var sourceBrowse = new ActionButton("Browse…") { Width = 122 };
        sourceRow.Controls.Add(sourceText, 0, 0); sourceRow.Controls.Add(sourceBrowse, 1, 0); files.Controls.Add(sourceRow);
        sourceText.TextChanged += (_, _) => { inputPath = sourceText.Text.Trim(); inputVerified = false; UpdateNavigation(); };
        sourceText.Leave += async (_, _) => { if (!string.IsNullOrWhiteSpace(inputPath) && !inputVerified && !scanning) await InspectInput(); };
        sourceBrowse.Click += async (_, _) =>
        {
            string? selected = null;
            if (inputKind == InputKind.Iso)
            {
                using var dialog = new OpenFileDialog { Title = "Choose The Simpsons Game Xbox 360 ISO", Filter = "Xbox 360 disc images (*.iso;*.xiso)|*.iso;*.xiso|All files (*.*)|*.*", CheckFileExists = true, Multiselect = false };
                if (dialog.ShowDialog(this) == DialogResult.OK) selected = dialog.FileName;
            }
            else
            {
                using var dialog = new FolderBrowserDialog { Description = "Choose the folder containing default.xex and movies", UseDescriptionForTitle = true };
                if (dialog.ShowDialog(this) == DialogResult.OK) selected = dialog.SelectedPath;
            }
            if (selected is null) return;
            sourceText.Text = selected;
            await InspectInput();
        };
        inputStatus = Theme.Label(scanning ? "Checking game files…" : inputVerified ? verifiedDescription : "Your original files are opened read-only.", 10, inputVerified, inputVerified ? Theme.Good : Theme.Muted);
        inputStatus.MaximumSize = new Size(650, 0); inputStatus.Margin = new Padding(0, 12, 0, 0); files.Controls.Add(inputStatus);

        TableLayoutPanel output = AddCard("2. Choose where to build", "The builder will create a new PPSA74207 folder inside this location.");
        var outputRow = new TableLayoutPanel { ColumnCount = 2, AutoSize = true, Dock = DockStyle.Top, Margin = Padding.Empty };
        outputRow.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100)); outputRow.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 130));
        var outputText = new TextBox { Text = outputParent, Dock = DockStyle.Fill, Font = Theme.Body(10), Margin = new Padding(0, 10, 8, 0), PlaceholderText = "Choose an output folder…" };
        outputText.TextChanged += (_, _) => { outputParent = outputText.Text.Trim(); UpdateNavigation(); };
        var outputBrowse = new ActionButton("Browse…") { Width = 122 };
        outputBrowse.Click += (_, _) =>
        {
            using var dialog = new FolderBrowserDialog { Description = "Choose a folder for your new PS5 build", UseDescriptionForTitle = true, ShowNewFolderButton = true };
            if (Directory.Exists(outputParent)) dialog.SelectedPath = outputParent;
            if (dialog.ShowDialog(this) == DialogResult.OK) outputText.Text = dialog.SelectedPath;
        };
        outputRow.Controls.Add(outputText, 0, 0); outputRow.Controls.Add(outputBrowse, 1, 0); output.Controls.Add(outputRow);
        var note = Theme.Label("Existing PPSA74207 folders are kept intact. Choose a new location for each build.", 9.5f, false, Theme.Muted);
        note.MaximumSize = new Size(650, 0); note.Margin = new Padding(0, 12, 0, 0); output.Controls.Add(note);
        TableLayoutPanel ready = AddCard("A familiar place to start", "The recommended profile uses 4K rendering, a 60 FPS target, original outlines and 16× texture filtering. You can customize it in the next step.");
        ready.BackColor = Color.FromArgb(255, 251, 226);
    }

    private async Task InspectInput()
    {
        scanCancellation?.Cancel(); scanCancellation?.Dispose();
        var cancellation = new CancellationTokenSource(); scanCancellation = cancellation;
        BuildInput selected = new(inputKind, inputPath);
        if (string.IsNullOrWhiteSpace(selected.Path)) return;
        var inspectionClock = Stopwatch.StartNew();
        diagnostics.Record("inspection.start", selected);
        scanning = true; inputVerified = false; UpdateNavigation();
        if (inputStatus is not null) { inputStatus.Text = "Checking the disc and game identity…"; inputStatus.ForeColor = Theme.Muted; }
        try
        {
            var inspection = await Task.Run(() =>
            {
                using GameSource source = GameSource.Open(selected, bundle.Manifest.XboxTitleId, cancellation.Token, log: diagnostics.Write);
                IReadOnlyList<EpisodeChoice> availableEpisodes = [];
                string? warning = null;
                try { availableEpisodes = GamePatches.ReadEpisodes(source, cancellation.Token); }
                catch (BuilderException e) { warning = e.Message; diagnostics.Error("inspection.episodes-unavailable", e); }
                return (source.FileCount, source.TotalBytes, Episodes: availableEpisodes, Warning: warning);
            }, cancellation.Token);
            if (IsDisposed || cancellation != scanCancellation || selected.Path != inputPath || selected.Kind != inputKind) return;
            inputVerified = true; episodes = inspection.Episodes; episodeWarning = inspection.Warning;
            if (startEpisode is not null && !episodes.Any(e => e.Id == startEpisode)) startEpisode = null;
            verifiedDescription = $"✓  The Simpsons Game confirmed  ·  {inspection.FileCount:N0} files  ·  {Format.Bytes(inspection.TotalBytes)}";
            diagnostics.Record("inspection.complete", new { inspection.FileCount, inspection.TotalBytes, episodes = episodes.Select(e => e.Id), warning = episodeWarning, durationMs = inspectionClock.ElapsedMilliseconds });
            if (inputStatus is not null) { inputStatus.Text = verifiedDescription; inputStatus.ForeColor = Theme.Good; }
        }
        catch (OperationCanceledException e) { diagnostics.Error("inspection.cancelled", e, new { durationMs = inspectionClock.ElapsedMilliseconds }); }
        catch (Exception e)
        {
            diagnostics.Error("inspection.failed", e, selected);
            if (IsDisposed || cancellation != scanCancellation) return;
            inputVerified = false;
            if (inputStatus is not null) { inputStatus.Text = e is BuilderException ? e.Message : "The game files could not be read. " + e.Message; inputStatus.ForeColor = Color.FromArgb(158, 52, 48); }
        }
        finally { if (!IsDisposed && cancellation == scanCancellation) { scanning = false; UpdateNavigation(); } }
    }

    private void GraphicsPage()
    {
        heading.Text = "MAKE SPRINGFIELD YOURS.";
        subtitle.Text = "Choose your graphics, game and audio settings. Experimental options are marked.";
        TableLayoutPanel profile = AddCard("Recommended: 4K / 60 FPS", "Fullscreen · Vulkan / RADV · DualSense controls");
        var reset = new ActionButton("Restore recommended", false) { Width = 235 };
        reset.Click += (_, _) => { settings = bundle.Capabilities.Defaults(); skipIntro = false; startEpisode = null; diagnostics.Record("settings.restore-recommended", settings); ShowStep(1); };
        profile.Controls.Add(reset);
        foreach (CapabilityGroup group in bundle.Capabilities.Groups.Where(g => g.Id != "language"))
        {
            TableLayoutPanel table = AddCard(group.Label, group.Description);
            if (group.RequiresFile == "lossless.dll") AddLosslessPicker(table);
            AddPresetPicker(table, group.Id);
            foreach (CapabilitySetting setting in bundle.Capabilities.Settings.Where(s => s.Group == group.Id)) AddSetting(table, setting);
            if (group.Id == "game") AddGamePatches(table);
        }
        RefreshDependencies();
    }

    private void LanguagePage()
    {
        heading.Text = "HELLO, SPRINGFIELD!";
        subtitle.Text = "Choose the language you want to hear and read in the game.";
        TableLayoutPanel table = AddCard("Your game language", "Choose a language included on your Xbox 360 disc.");
        foreach (CapabilitySetting setting in bundle.Capabilities.Settings.Where(s => s.Group == "language")) AddSetting(table, setting);
        AddCard("Using an Italian / Spanish disc?", "Select Italian or Spanish to use the languages included in that edition. The builder keeps the game’s original dialogue and text files.");
    }

    private void AddSetting(TableLayoutPanel group, CapabilitySetting setting)
    {
        var row = new TableLayoutPanel { AutoSize = true, Dock = DockStyle.Top, ColumnCount = 2, Padding = new Padding(0, 15, 0, 12), Margin = Padding.Empty };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100)); row.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 250));
        var labels = new FlowLayoutPanel { AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink, FlowDirection = FlowDirection.TopDown, WrapContents = false, Dock = DockStyle.Top, Margin = new Padding(0, 0, 15, 0) };
        var label = Theme.Label(setting.Label, 10.5f, true); labels.Controls.Add(label);
        var description = Theme.Label(setting.Description, 9.2f, false, Theme.Muted); description.MaximumSize = new Size(350, 0); labels.Controls.Add(description);
        var experimental = Theme.Label("EXPERIMENTAL", 8, true, Color.FromArgb(145, 94, 0));
        labels.Controls.Add(experimental); experimentalLabels[setting.Key] = experimental;
        row.Controls.Add(labels, 0, 0);
        Control control;
        JsonElement current = settings[setting.Key];
        switch (setting.Type)
        {
            case "choice":
                var combo = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Width = 244, DropDownWidth = 430, Font = Theme.Body(9.7f), IntegralHeight = false, MaxDropDownItems = 12, Margin = new Padding(0, 2, 0, 0) };
                foreach (CapabilityChoice choice in setting.Choices) combo.Items.Add(choice);
                combo.SelectedItem = setting.Choices.First(c => Capabilities.Equal(c.Value, current));
                combo.SelectedIndexChanged += (_, _) => { if (!syncing && combo.SelectedItem is CapabilityChoice choice) SetSetting(setting.Key, choice.Value); };
                editorWriters[setting.Key] = value => combo.SelectedItem = setting.Choices.First(c => Capabilities.Equal(c.Value, value)); control = combo;
                break;
            case "boolean":
                var check = new CheckBox { Checked = current.GetBoolean(), Text = current.GetBoolean() ? "On" : "Off", AutoSize = true, Font = Theme.Body(11), Margin = new Padding(0, 3, 0, 0) };
                check.CheckedChanged += (_, _) => { check.Text = check.Checked ? "On" : "Off"; if (!syncing) SetSetting(setting.Key, JsonSerializer.SerializeToElement(check.Checked)); };
                editorWriters[setting.Key] = value => check.Checked = value.GetBoolean(); control = check;
                break;
            case "number":
                var numeric = new NumericUpDown { Minimum = setting.Min ?? -1000, Maximum = setting.Max ?? 1000, Increment = setting.Step ?? 1,
                    DecimalPlaces = setting.Step.HasValue && setting.Step < 1 ? 2 : 0, Value = current.GetDecimal(), Width = 160, Font = Theme.Body(11), Margin = new Padding(0, 3, 0, 0) };
                numeric.ValueChanged += (_, _) => { if (!syncing) SetSetting(setting.Key, JsonSerializer.SerializeToElement(numeric.Value)); };
                editorWriters[setting.Key] = value => numeric.Value = value.GetDecimal(); control = numeric;
                break;
            case "color":
                var colorPanel = new FlowLayoutPanel { AutoSize = true, Width = 245, WrapContents = false, Margin = Padding.Empty };
                var hex = new TextBox { ReadOnly = true, Width = 104, Font = Theme.Body(10, FontStyle.Bold), TextAlign = HorizontalAlignment.Center, Margin = new Padding(0, 11, 8, 0) };
                var choose = new ActionButton("Choose…") { Width = 122 };
                void UpdateColour(JsonElement value)
                {
                    hex.Text = "#" + value.GetString()!.ToUpperInvariant(); hex.BackColor = ColorTranslator.FromHtml(hex.Text);
                    hex.ForeColor = hex.BackColor.GetBrightness() < 0.5f ? Color.White : Theme.Ink;
                }
                UpdateColour(current);
                choose.Click += (_, _) =>
                {
                    using var dialog = new ColorDialog { Color = ColorTranslator.FromHtml("#" + settings[setting.Key].GetString()), FullOpen = true, AnyColor = true };
                    if (dialog.ShowDialog(this) == DialogResult.OK)
                    {
                        JsonElement value = JsonSerializer.SerializeToElement($"{dialog.Color.R:X2}{dialog.Color.G:X2}{dialog.Color.B:X2}");
                        UpdateColour(value); SetSetting(setting.Key, value);
                    }
                };
                colorPanel.Controls.Add(hex); colorPanel.Controls.Add(choose);
                editorWriters[setting.Key] = UpdateColour; control = colorPanel;
                break;
            default: throw new BuilderException("Unsupported bundled setting type: " + setting.Type);
        }
        control.AccessibleName = setting.Label; editors[setting.Key] = control; row.Controls.Add(control, 1, 0); group.Controls.Add(row);
        labels.SizeChanged += (_, _) =>
        {
            description.MaximumSize = new Size(Math.Max(1, labels.ClientSize.Width - labels.Padding.Horizontal), 0);
            label.MaximumSize = description.MaximumSize;
        };
    }

    private void AddPresetPicker(TableLayoutPanel table, string group)
    {
        List<CapabilityPreset> presets = bundle.Capabilities.Presets.Where(p => p.Group == group).ToList();
        if (presets.Count == 0) return;
        var panel = new FlowLayoutPanel { AutoSize = true, FlowDirection = FlowDirection.LeftToRight, Margin = new Padding(0, 7, 0, 0) };
        var label = Theme.Label("Preset", 10, true); label.Margin = new Padding(0, 5, 20, 0); panel.Controls.Add(label);
        var combo = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Width = 220, Font = Theme.Body(10) };
        combo.Items.Add("Custom"); foreach (CapabilityPreset preset in presets) combo.Items.Add(preset);
        combo.SelectedIndex = 0;
        combo.SelectedIndexChanged += (_, _) =>
        {
            if (syncing || combo.SelectedItem is not CapabilityPreset preset) return;
            diagnostics.Record("settings.preset", new { preset.Id, preset.Values });
            syncing = true;
            foreach ((string key, JsonElement value) in preset.Values)
            {
                settings[key] = value.Clone(); if (editorWriters.TryGetValue(key, out Action<JsonElement>? writer)) writer(value);
            }
            syncing = false; RefreshDependencies();
        };
        presetCombos.Add((group, combo)); panel.Controls.Add(combo); table.Controls.Add(panel);
    }

    private void SetSetting(string key, JsonElement value) { settings[key] = value.Clone(); diagnostics.Record("settings.change", new { key, value }); RefreshDependencies(); }

    private void RefreshDependencies()
    {
        foreach (CapabilitySetting setting in bundle.Capabilities.Settings)
        {
            bool needsLibrary = bundle.Capabilities.Groups.Any(g => g.Id == setting.Group && g.RequiresFile == "lossless.dll");
            if (editors.TryGetValue(setting.Key, out Control? editor))
                editor.Enabled = (!needsLibrary || losslessAvailable) && bundle.Capabilities.IsEnabled(setting, settings) && setting.ApplyMode != "fixed";
            if (experimentalLabels.TryGetValue(setting.Key, out Label? label))
                label.Visible = setting.Status == "experimental" || setting.Choices.Any(c => c.Status == "experimental" && Capabilities.Equal(c.Value, settings[setting.Key]));
        }
        syncing = true;
        foreach ((string group, ComboBox combo) in presetCombos)
        {
            CapabilityPreset? active = bundle.Capabilities.Presets.FirstOrDefault(p => p.Group == group && p.Values.All(v => Capabilities.Equal(settings[v.Key], v.Value)));
            if (active is null) combo.SelectedIndex = 0; else combo.SelectedItem = active;
        }
        syncing = false;
    }

    private void AddLosslessPicker(TableLayoutPanel table)
    {
        var note = Theme.Label("Optional: choose your own Lossless.dll to unlock the controls below.", 10, false, Theme.Muted);
        note.MaximumSize = new Size(640, 0); table.Controls.Add(note);
        var row = new TableLayoutPanel { AutoSize = true, Dock = DockStyle.Top, ColumnCount = 3, Margin = Padding.Empty };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 123)); row.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 105));
        var path = new TextBox { Text = losslessPath ?? "", ReadOnly = true, Dock = DockStyle.Fill, Font = Theme.Body(9.5f), Margin = new Padding(0, 12, 8, 0), PlaceholderText = "No Lossless.dll selected" };
        var choose = new ActionButton("Choose…") { Width = 115 };
        var remove = new ActionButton("Remove") { Width = 95, Enabled = losslessAvailable };
        var status = Theme.Label(losslessAvailable ? "✓  File selected. Frame generation controls are available." : "No file selected. Frame generation is off.", 9.5f, false, losslessAvailable ? Theme.Good : Theme.Muted);
        status.MaximumSize = new Size(640, 0); status.Margin = new Padding(0, 8, 0, 0);
        choose.Click += (_, _) =>
        {
            using var dialog = new OpenFileDialog { Title = "Choose your Lossless.dll", Filter = "Lossless DLL (Lossless.dll)|Lossless.dll|DLL files (*.dll)|*.dll|All files (*.*)|*.*", CheckFileExists = true, Multiselect = false };
            if (dialog.ShowDialog(this) != DialogResult.OK) return;
            try
            {
                using OptionalLibrary selected = OptionalLibrary.Open(dialog.FileName);
                losslessPath = selected.FullPath; losslessAvailable = true; path.Text = losslessPath; remove.Enabled = true;
                diagnostics.Record("optional-library.selected", new { selected.FullPath, selected.Length, validation = "ordinary file access only" });
                status.Text = "✓  File selected. Frame generation controls are available."; status.ForeColor = Theme.Good;
                RefreshDependencies();
            }
            catch (Exception e)
            {
                diagnostics.Error("optional-library.selection-failed", e, new { path = dialog.FileName });
                status.Text = "The file could not be opened. " + e.Message; status.ForeColor = Color.FromArgb(158, 52, 48);
            }
        };
        remove.Click += (_, _) =>
        {
            diagnostics.Record("optional-library.removed", new { frameGeneration = "restored defaults" });
            losslessPath = null; losslessAvailable = false; path.Text = ""; remove.Enabled = false;
            syncing = true;
            foreach (CapabilitySetting setting in bundle.Capabilities.Settings.Where(s => bundle.Capabilities.Groups.Any(g => g.Id == s.Group && g.RequiresFile == "lossless.dll")))
            {
                settings[setting.Key] = setting.Default.Clone();
                if (editorWriters.TryGetValue(setting.Key, out Action<JsonElement>? writer)) writer(setting.Default);
            }
            syncing = false; RefreshDependencies();
            status.Text = "No file selected. Frame generation is off."; status.ForeColor = Theme.Muted;
        };
        row.Controls.Add(path, 0, 0); row.Controls.Add(choose, 1, 0); row.Controls.Add(remove, 2, 0); table.Controls.Add(row); table.Controls.Add(status);
    }

    private void AddGamePatches(TableLayoutPanel table)
    {
        var skip = new CheckBox { Text = "Skip intro logo videos", Checked = skipIntro, AutoSize = true, Font = Theme.Body(10.5f, FontStyle.Bold), Margin = new Padding(0, 16, 0, 9) };
        skip.CheckedChanged += (_, _) => { skipIntro = skip.Checked; diagnostics.Record("game-options.skip-intro", new { skipIntro }); }; table.Controls.Add(skip);
        var skipNote = Theme.Label("Keeps the original logo files in your new game folder so the change can be reversed.", 9.2f, false, Theme.Muted);
        skipNote.MaximumSize = new Size(640, 0); table.Controls.Add(skipNote);
        var episodeTitle = Theme.Label("Starting episode  ·  Experimental", 10.5f, true); episodeTitle.Margin = new Padding(0, 16, 0, 8); table.Controls.Add(episodeTitle);
        var episodeBox = new ComboBox { Width = 460, DropDownStyle = ComboBoxStyle.DropDownList, DropDownWidth = 540, Font = Theme.Body(10) };
        episodeBox.Items.Add("Keep disc / folder starting episode"); foreach (EpisodeChoice episode in episodes) episodeBox.Items.Add(new EpisodeItem(episode));
        episodeBox.SelectedIndex = 0;
        for (int i = 1; i < episodeBox.Items.Count; i++) if (episodeBox.Items[i] is EpisodeItem candidate && candidate.Choice.Id == startEpisode) episodeBox.SelectedIndex = i;
        episodeBox.SelectedIndexChanged += (_, _) => { startEpisode = episodeBox.SelectedItem is EpisodeItem selected ? selected.Choice.Id : null; diagnostics.Record("game-options.start-episode", new { startEpisode }); };
        episodeBox.Enabled = episodes.Count > 0; table.Controls.Add(episodeBox);
        var episodeNote = Theme.Label("New games only; existing saves keep their progress.", 9.2f, false, Theme.Muted); episodeNote.Margin = new Padding(0, 9, 0, 0); table.Controls.Add(episodeNote);
        if (episodeWarning is not null)
        {
            var warning = Theme.Label("Level select is unavailable for these files. " + episodeWarning, 9.2f, false, Theme.Muted);
            warning.MaximumSize = new Size(640, 0); table.Controls.Add(warning);
        }
    }

    private sealed record EpisodeItem(EpisodeChoice Choice) { public override string ToString() => Choice.Label; }

    private void BuildPage()
    {
        if (result is not null)
        {
            heading.Text = "WOO-HOO! YOU’RE READY.";
            subtitle.Text = "Your PS5 folder is complete and every copied file passed verification.";
            TableLayoutPanel done = AddCard("The Simpsons Game", result.OutputDirectory);
            done.Controls.Add(Theme.Label($"{result.FileCount:N0} game files  ·  {Format.Bytes(result.GameBytes)}  ·  {result.Duration.TotalMinutes:0.0} minutes", 10, true, Theme.Good));
            var open = new ActionButton("Open output folder", true) { Width = 230 };
            open.Click += (_, _) => OpenPath(result.OutputDirectory); done.Controls.Add(open);
            AddCard("Next stop: your PS5", "Copy the complete PPSA74207 folder to /data/homebrew/PPSA74207. Mount it with ShadowMountPlus and launch The Simpsons Game from the PS5 home screen. Keep userdata when you update.");
            AddLogCard(); return;
        }
        heading.Text = "NEXT STOP: SPRINGFIELD.";
        subtitle.Text = running ? "The builder is preparing and checking your PS5 folder." : "Check your choices, then build. Your source disc image will remain unchanged.";
        TableLayoutPanel summary = AddCard("Ready to build", "The Simpsons Game  ·  PPSA74207  ·  PS5 " + bundle.Manifest.BuildVersion);
        AddSummary(summary, "Game files", inputPath);
        AddSummary(summary, "Output folder", Path.Combine(outputParent, "PPSA74207"));
        if (losslessAvailable) AddSummary(summary, "Lossless.dll", "Your selected file will be included");
        foreach (string key in new[] { "resolution_scale", "frame_rate", "user_language" })
        {
            CapabilitySetting? setting = bundle.Capabilities.Settings.FirstOrDefault(s => s.Key == key);
            if (setting is not null) AddSummary(summary, setting.Label, ValueLabel(setting));
        }
        var experimental = bundle.Capabilities.ExperimentalSelections(settings).ToList();
        if (startEpisode is not null) experimental.Add("Starting episode");
        if (experimental.Count > 0)
        {
            var note = Theme.Label("Experimental choices: " + string.Join(", ", experimental), 9.5f, false, Color.FromArgb(139, 91, 0));
            note.MaximumSize = new Size(650, 0); note.Margin = new Padding(0, 14, 0, 0); summary.Controls.Add(note);
        }
        TableLayoutPanel progress = AddCard("Build progress", "Extract game data → Prepare PS5 title → Verify files → Finish");
        progressPhase = Theme.Label(running ? "Starting…" : "Ready when you are.", 11, true); progress.Controls.Add(progressPhase);
        progressBar = new ProgressBar { Style = ProgressBarStyle.Continuous, Maximum = 1000, Height = 20, Dock = DockStyle.Top, Margin = new Padding(0, 4, 0, 10) }; progress.Controls.Add(progressBar);
        progressFile = Theme.Label("", 9.5f, false, Theme.Muted); progressFile.AutoSize = false; progressFile.Height = 25; progressFile.Dock = DockStyle.Top; progressFile.AutoEllipsis = true; progress.Controls.Add(progressFile);
        progressNumbers = Theme.Label("", 9.5f, false, Theme.Muted); progress.Controls.Add(progressNumbers);
        outcomeLabel = Theme.Label("", 10, true); outcomeLabel.MaximumSize = new Size(650, 0); progress.Controls.Add(outcomeLabel);
        AddLogCard();
    }

    private void AddSummary(TableLayoutPanel parent, string name, string value)
    {
        var row = new TableLayoutPanel { AutoSize = true, Dock = DockStyle.Top, ColumnCount = 2, Margin = new Padding(0, 4, 0, 3) };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 154)); row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        row.Controls.Add(Theme.Label(name, 9.7f, true), 0, 0);
        var text = Theme.Label(value, 9.7f); text.MaximumSize = new Size(500, 0); row.Controls.Add(text, 1, 0);
        row.SizeChanged += (_, _) => text.MaximumSize = new Size(Math.Max(1, row.ClientSize.Width - (int)row.ColumnStyles[0].Width - text.Margin.Horizontal), 0);
        parent.Controls.Add(row);
    }

    private string ValueLabel(CapabilitySetting setting) => setting.Choices.FirstOrDefault(c => Capabilities.Equal(c.Value, settings[setting.Key]))?.Label ?? Capabilities.TomlValue(settings[setting.Key]);

    private void AddLogCard()
    {
        TableLayoutPanel details = AddCard("Build details & diagnostics", "A detailed local log starts as soon as the builder opens. It records steps, timings, settings, file checks and errors.");
        var reveal = new LinkLabel { Text = "Show details", AutoSize = true, LinkColor = Theme.Sky, Font = Theme.Body(10), Margin = new Padding(0, 0, 0, 8) };
        buildDetails = new TextBox { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, Font = new Font("Consolas", 9), Height = 145, Dock = DockStyle.Top, Visible = false,
            Text = diagnostics.RecentText(80), BackColor = Color.FromArgb(247, 250, 253) };
        reveal.LinkClicked += (_, _) => { buildDetails.Visible = !buildDetails.Visible; reveal.Text = buildDetails.Visible ? "Hide details" : "Show details"; RefreshLogPreview(); };
        details.Controls.Add(reveal); details.Controls.Add(buildDetails);
        details.Controls.Add(DiagnosticActions());
        var location = Theme.Label(diagnostics.LocationDescription, 8.5f, false, Theme.Muted);
        location.MaximumSize = new Size(650, 0); details.Controls.Add(location);
    }

    private async Task StartBuild()
    {
        if (!inputVerified || string.IsNullOrWhiteSpace(outputParent)) { ShowStep(0); return; }
        running = true; result = null; buildCancellation = new CancellationTokenSource();
        diagnostics.Record("ui.build-requested", new { buildId = Guid.NewGuid().ToString("N"), inputKind, inputPath, outputParent });
        try
        {
            ShowStep(3);
            long lastProgressLog = 0; string lastProgressPhase = "";
            var progressClock = Stopwatch.StartNew();
            var progress = new Progress<BuildProgress>(p =>
            {
                if (IsDisposed || !running) return;
                if (p.Phase != lastProgressPhase || progressClock.ElapsedMilliseconds - lastProgressLog >= 1000)
                {
                    lastProgressLog = progressClock.ElapsedMilliseconds; lastProgressPhase = p.Phase;
                    diagnostics.Record("ui.progress", p);
                }
                if (progressBar is not null) progressBar.Value = Math.Clamp((int)(p.Percent * 10), 0, 1000);
                if (progressPhase is not null) progressPhase.Text = p.Message;
                if (progressFile is not null) progressFile.Text = p.CurrentFile ?? "";
                if (progressNumbers is not null) progressNumbers.Text = p.BytesTotal > 0 ? $"{Format.Bytes(p.BytesDone)} / {Format.Bytes(p.BytesTotal)}  ·  {p.FilesDone:N0} / {p.FilesTotal:N0} files" : "";
            });
            var request = new BuildRequest(new(inputKind, inputPath), outputParent, settings.ToDictionary(p => p.Key, p => p.Value.Clone()), new GamePatchOptions(skipIntro, startEpisode), losslessPath);
            result = await new BuildPipeline(bundle).BuildAsync(request, progress, diagnostics.Write, buildCancellation.Token);
            diagnostics.Record("ui.build-complete", result); await diagnostics.FlushAsync();
            running = false; ShowStep(3);
        }
        catch (OperationCanceledException)
        {
            diagnostics.Record("ui.build-cancelled"); await diagnostics.FlushAsync();
            if (progressPhase is not null) progressPhase.Text = "Build cancelled.";
            if (outcomeLabel is not null) { outcomeLabel.Text = "Your original files and existing output folders have been kept."; outcomeLabel.ForeColor = Theme.Muted; }
        }
        catch (Exception e)
        {
            diagnostics.Error("ui.build-failed", e); await diagnostics.FlushAsync();
            if (progressPhase is not null) progressPhase.Text = "The build could not be completed.";
            if (outcomeLabel is not null) { outcomeLabel.Text = e.Message; outcomeLabel.ForeColor = Color.FromArgb(158, 52, 48); }
            MessageBox.Show(this, e.Message + "\n\nDiagnostics:\n" + diagnostics.LocationDescription,
                "Build could not be completed", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally
        {
            running = false; buildCancellation.Dispose(); buildCancellation = null; UpdateNavigation(); RefreshLogPreview();
            if (closeRequested) Close();
        }
    }

    private TableLayoutPanel AddCard(string title, string description = "")
    {
        var card = new Card { Width = Math.Max(Dpi(480), content.ClientSize.Width - content.Padding.Horizontal - SystemInformation.VerticalScrollBarWidth - Dpi(3)) };
        var table = new TableLayoutPanel { ColumnCount = 1, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink, Dock = DockStyle.Top, Margin = Padding.Empty };
        table.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        if (title.Length > 0) table.Controls.Add(Theme.Label(title, 13, true));
        if (description.Length > 0)
        {
            var text = Theme.Label(description, 10, false, Theme.Muted); text.MaximumSize = new Size(650, 0); table.Controls.Add(text);
            table.SizeChanged += (_, _) => text.MaximumSize = new Size(Math.Max(100, table.ClientSize.Width), 0);
        }
        card.Controls.Add(table); content.Controls.Add(card); return table;
    }

    private void FitCards()
    {
        if (content is null) return;
        int width = Math.Max(Dpi(480), content.ClientSize.Width - content.Padding.Horizontal - SystemInformation.VerticalScrollBarWidth - Dpi(3));
        foreach (Control card in content.Controls) card.Width = width;
    }

    private int Dpi(int logical) => (int)Math.Round(logical * DeviceDpi / 96.0);

    private void FitWindowToWorkingArea()
    {
        if (WindowState != FormWindowState.Normal) return;
        Rectangle area = Screen.FromHandle(Handle).WorkingArea;
        MinimumSize = new Size(Math.Min(Dpi(1050), area.Width), Math.Min(Dpi(730), area.Height));
        Size = new Size(Math.Min(Width, area.Width), Math.Min(Height, area.Height));
        Location = new Point(Math.Clamp(Left, area.Left, area.Right - Width), Math.Clamp(Top, area.Top, area.Bottom - Height));
    }

    private void UpdateNavigation()
    {
        back.Enabled = !running && !scanning && currentStep > 0 && result is null;
        next.Enabled = !running && !scanning && (result is not null || (inputVerified && !string.IsNullOrWhiteSpace(outputParent)));
        next.Text = result is not null ? "Build another" : currentStep switch { 0 => "Next: Graphics  →", 1 => "Next: Language  →", 2 => "Review & build  →", _ => "Build PS5 folder" };
        cancel.Text = running ? "Cancel" : "Close"; cancel.Enabled = true;
        foreach (StepButton step in steps) step.Enabled = !running && !scanning && (step.Number == 1 || inputVerified);
    }

    private void ShowCredits()
    {
        using var dialog = new Form { Text = "About & credits", StartPosition = FormStartPosition.CenterParent, Size = new Size(720, 580), MinimumSize = new Size(550, 420), Font = Theme.Body(10) };
        dialog.Controls.Add(new TextBox { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, Dock = DockStyle.Fill,
            Text = "The Simpsons Game PS5 Builder\r\nBuilder 1.0 · PS5 " + bundle.Manifest.BuildVersion + "\r\n\r\nRuntime included — no separate .NET installation required.\r\nA local builder for your own Xbox 360 disc dump.\r\n\r\nSession log:\r\n" + diagnostics.LocationDescription + "\r\n\r\n" + Program.Notices().ReplaceLineEndings("\r\n"), BackColor = Color.White });
        FlowLayoutPanel actions = DiagnosticActions(); actions.Dock = DockStyle.Bottom; dialog.Controls.Add(actions);
        dialog.ShowDialog(this);
    }

    private FlowLayoutPanel DiagnosticActions()
    {
        var actions = new FlowLayoutPanel { AutoSize = true, Dock = DockStyle.Top, WrapContents = true, Margin = new Padding(0, 7, 0, 3) };
        var open = new ActionButton("Open logs") { Width = 145 }; open.Click += (_, _) => OpenLogs();
        var copy = new ActionButton("Copy diagnostics") { Width = 185 };
        copy.Click += async (_, _) =>
        {
            try { Clipboard.SetText(await diagnostics.DiagnosticTextAsync()); copy.Text = "Copied!"; }
            catch (Exception e) { diagnostics.Error("diagnostics.copy-failed", e); MessageBox.Show(this, "Diagnostics could not be copied.\n\n" + e.Message + "\n\n" + diagnostics.LocationDescription, "Diagnostics", MessageBoxButtons.OK, MessageBoxIcon.Information); }
        };
        var export = new ActionButton("Export full log…") { Width = 180 };
        export.Click += async (_, _) =>
        {
            using var dialog = new SaveFileDialog { Title = "Export builder diagnostics", Filter = "Diagnostic log (*.log)|*.log", OverwritePrompt = false,
                FileName = "Simpsons-Builder-" + DateTimeOffset.UtcNow.ToString("yyyyMMdd-HHmmss") + ".log" };
            if (dialog.ShowDialog(this) != DialogResult.OK) return;
            try
            {
                await diagnostics.ExportAsync(dialog.FileName); diagnostics.Record("diagnostics.exported", new { path = dialog.FileName });
                MessageBox.Show(this, "Diagnostics exported to:\n" + dialog.FileName, "Diagnostics", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            catch (Exception e) { diagnostics.Error("diagnostics.export-failed", e); MessageBox.Show(this, "Diagnostics could not be exported. Choose a new file name in a writable folder.\n\n" + e.Message, "Diagnostics", MessageBoxButtons.OK, MessageBoxIcon.Information); }
        };
        actions.Controls.AddRange([open, copy, export]); return actions;
    }

    private void OpenLogs()
    {
        if (diagnostics.LogPath is not null) OpenPath(Path.GetDirectoryName(diagnostics.LogPath)!);
        else MessageBox.Show(this, diagnostics.LocationDescription + "\n\n" + diagnostics.StorageProblem, "Diagnostics", MessageBoxButtons.OK, MessageBoxIcon.Information);
    }

    private void RefreshLogPreview()
    {
        if (buildDetails is null || buildDetails.IsDisposed || !buildDetails.Visible || displayedLogSequence == diagnostics.LastSequence) return;
        displayedLogSequence = diagnostics.LastSequence;
        buildDetails.Text = diagnostics.RecentText(80); buildDetails.SelectionStart = buildDetails.TextLength; buildDetails.ScrollToCaret();
    }

    private void OpenPath(string path)
    {
        try { Process.Start(new ProcessStartInfo(path) { UseShellExecute = true }); }
        catch (Exception e) { diagnostics.Error("ui.open-path-failed", e, new { path }); MessageBox.Show(this, "The file or folder could not be opened.\n\n" + path + "\n\n" + e.Message + "\n\n" + diagnostics.LocationDescription, "Open file", MessageBoxButtons.OK, MessageBoxIcon.Information); }
    }

    internal void SavePreview(string directory)
    {
        Directory.CreateDirectory(directory);
        for (int i = 0; i < 4; i++)
        {
            ShowStep(i); PerformLayout(); Application.DoEvents();
            using var bitmap = new Bitmap(Width, Height);
            DrawToBitmap(bitmap, new Rectangle(0, 0, Width, Height));
            bitmap.Save(Path.Combine(directory, $"step-{i + 1}.png"), ImageFormat.Png);
        }
        File.WriteAllText(Path.Combine(directory, "preview.json"), JsonSerializer.Serialize(new { result = "PASS", pages = 4, title = Text, width = Width, height = Height, dpi = DeviceDpi, workingArea = Screen.FromHandle(Handle).WorkingArea, windows = Environment.OSVersion.ToString() }, JsonFiles.Options));
    }

    private static Icon CreateAppIcon(Image image)
    {
        using var bitmap = new Bitmap(image, new Size(64, 64));
        IntPtr handle = bitmap.GetHicon();
        try { using Icon borrowed = Icon.FromHandle(handle); return (Icon)borrowed.Clone(); }
        finally { DestroyIcon(handle); }
    }
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern bool DestroyIcon(IntPtr icon);

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            logRefresh.Stop(); logRefresh.Dispose();
            scanCancellation?.Cancel(); scanCancellation?.Dispose(); buildCancellation?.Dispose();
            cover.Dispose(); titleFont.Dispose();
        }
        base.Dispose(disposing);
    }
}
