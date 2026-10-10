// Windows-only audit of the production WPF assembly, BAML, resources and controls.
// Run at the monitor's real scaling. RenderTargetBitmap DPI is NOT used to fake DPI changes.
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text.Json;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using Simpsons.Builder;
using Simpsons.Builder.Desktop;

namespace Simpsons.Builder.UiTests;

internal static class Program
{
    private static readonly JsonSerializerOptions JsonOptions = new() { WriteIndented = true };
    private static readonly List<object> Captures = [];
    private static readonly List<object> Issues = [];
    private static readonly BindingListener Bindings = new();
    private static string destination = "";
    private static int serial;
    private static bool galleryMode;
    private static bool frameGenerationUnavailableVerified;

    [STAThread]
    private static int Main(string[] args)
    {
        galleryMode = args.Length > 0 && args[0] == "--gallery";
        string[] positional = galleryMode ? args[1..] : args;
        if (positional.Length is < 1 or > 2 || (positional.Length == 2 && !int.TryParse(positional[1], out _)))
        { Console.Error.WriteLine("Builder.UiTests [--gallery] <new-output-directory> [expected-monitor-dpi: 96|120|144|192]"); return 2; }
        destination = Path.GetFullPath(positional[0]);
        if (Directory.Exists(destination))
        { Console.Error.WriteLine("Choose a new output directory so previous captures are preserved."); return 2; }
        Directory.CreateDirectory(destination);
        if (!OperatingSystem.IsWindows())
        { WriteReport("NOT_RUN", "WPF needs Windows. This harness does not emulate Windows layout or substitute screenshots."); return 3; }
        int? expectedDpi = positional.Length == 2 ? int.Parse(positional[1], CultureInfo.InvariantCulture) : null;
        int exitCode = 1;
        using DiagnosticSession diagnostics = DiagnosticSession.Start(Path.Combine(destination, "logs"));
        try
        {
            CultureInfo.DefaultThreadCurrentCulture = CultureInfo.GetCultureInfo("en-US");
            CultureInfo.DefaultThreadCurrentUICulture = CultureInfo.GetCultureInfo("en-US");
            PresentationTraceSources.DataBindingSource.Listeners.Add(Bindings);
            PresentationTraceSources.DataBindingSource.Switch.Level = SourceLevels.Warning;
            var app = new App();
            app.InitializeComponent();
            app.ShutdownMode = ShutdownMode.OnExplicitShutdown;
            Assembly production = typeof(MainWindow).Assembly;
            Type bootstrap = production.GetType("Simpsons.Builder.Desktop.Program", throwOnError: true)!;
            BundlePackage bundle = (BundlePackage)bootstrap.GetMethod("LoadBundle", BindingFlags.Static | BindingFlags.NonPublic)!
                .Invoke(null, [diagnostics])!;
            var window = new MainWindow(bundle, diagnostics);
            bool started = false;
            window.ContentRendered += async (_, _) =>
            {
                if (started) return;
                started = true;
                try
                {
                    DpiScale dpi = VisualTreeHelper.GetDpi(window);
                    if (expectedDpi.HasValue && Math.Abs(dpi.PixelsPerInchX - expectedDpi.Value) > 1)
                    {
                        WriteReport("NOT_RUN", $"Expected physical monitor DPI {expectedDpi}; actual DPI is {dpi.PixelsPerInchX}. Change Windows display scaling, then rerun in a new directory.");
                        exitCode = 3;
                        return;
                    }
                    if (galleryMode) await CaptureGallery(window);
                    else await Exercise(window, diagnostics);
                    if (Bindings.Lines.Count > 0) Issues.Add(new { severity = "error", kind = "wpf-binding-warning", lines = Bindings.Lines });
                    exitCode = Issues.Count == 0 ? 0 : 1;
                    WriteReport(exitCode == 0 ? "MEASURED_NO_FINDINGS" : "REVIEW_REQUIRED",
                        "Actual WPF controls and templates measured at the real monitor DPI. Inspect screenshots; geometric checks alone cannot certify readability, native Acrylic composition, mouse/keyboard behavior or PS5 operation.");
                }
                catch (Exception error)
                {
                    diagnostics.Error("ui-harness.failed", error);
                    Issues.Add(new { severity = "error", kind = "runtime-exception", exception = error.ToString() });
                    WriteReport("FAILED", "The native WPF audit did not complete.");
                }
                finally { window.Close(); app.Shutdown(); }
            };
            app.Run(window);
            return exitCode;
        }
        catch (Exception error)
        {
            diagnostics.Error("ui-harness.bootstrap-failed", error);
            Issues.Add(new { severity = "error", kind = "bootstrap-exception", exception = error.ToString() });
            WriteReport("FAILED", "The real application could not be initialized.");
            return 1;
        }
        finally { PresentationTraceSources.DataBindingSource.Listeners.Remove(Bindings); }
    }

    private static async Task CaptureGallery(MainWindow window)
    {
        MethodInfo showPage = typeof(MainWindow).GetMethod("ShowPage", BindingFlags.Instance | BindingFlags.NonPublic)!;
        // This is an ordinary resizable production window, rendered at the monitor's
        // real DPI. No screenshot is assembled from individual or replacement controls.
        window.WindowState = WindowState.Normal;
        window.Width = 1220; window.Height = 980;
        showPage.Invoke(window, [0]);
        await Capture(window, "source");

        showPage.Invoke(window, [1]);
        window.SelectedGroup = window.SettingGroups.Single(group => group.Id == "quality");
        ((ScrollViewer)window.FindName("PageScroll")).ScrollToTop();
        await Capture(window, "graphics");

        SettingGroupViewModel framegen = window.SettingGroups.Single(group => group.Id == "framegen");
        window.SelectedGroup = framegen;
        ((ScrollViewer)window.FindName("PageScroll")).ScrollToTop();
        await Settle(window);
        frameGenerationUnavailableVerified = !window.LosslessAvailable &&
            framegen.Settings.All(setting => !setting.IsEnabled) &&
            !framegen.Settings.Single(setting => setting.Key == "frame_gen").IsOn;
        if (!frameGenerationUnavailableVerified)
            throw new InvalidOperationException("Gallery capture must show real unavailable frame generation without a Lossless.dll. No file or availability state is injected.");
        TextBlock hint = Descendants(window).OfType<TextBlock>().Single(block =>
            block.IsVisible && block.Text.Contains("L1 + R1 + L3 + R3", StringComparison.Ordinal));
        hint.BringIntoView();
        await Capture(window, "frame-generation");
    }

    private static async Task Exercise(MainWindow window, DiagnosticSession diagnostics)
    {
        MethodInfo showPage = typeof(MainWindow).GetMethod("ShowPage", BindingFlags.Instance | BindingFlags.NonPublic)!;
        double normalMinWidth = window.MinWidth, normalMinHeight = window.MinHeight;
        var sizes = new[] { (Width: 608d, Height: 480d), (Width: normalMinWidth, Height: normalMinHeight),
            (Width: 1024d, Height: 720d), (Width: 1220d, Height: 840d) };
        foreach (var size in sizes)
        {
            window.WindowState = WindowState.Normal;
            // Exercise the production compact layout using a real constrained viewport.
            // Only the test window's minimum changes; monitor DPI and application code do not.
            window.MinWidth = Math.Min(normalMinWidth, size.Width);
            window.MinHeight = Math.Min(normalMinHeight, size.Height);
            window.Width = size.Width; window.Height = size.Height;
            await Settle(window);
            string tag = $"{size.Width:0}x{size.Height:0}";
            for (int page = 0; page < 4; page++)
            {
                showPage.Invoke(window, [page]);
                if (page == 1)
                {
                    foreach (SettingGroupViewModel group in window.SettingGroups)
                    {
                        window.SelectedGroup = group;
                        await Capture(window, $"{tag}-settings-{group.Id}");
                        ScrollViewer scroll = (ScrollViewer)window.FindName("PageScroll");
                        if (scroll.ScrollableHeight > 1)
                        { scroll.ScrollToEnd(); await Capture(window, $"{tag}-settings-{group.Id}-bottom"); }
                    }
                }
                else await Capture(window, $"{tag}-page-{page + 1}");
            }
        }
        window.MinWidth = normalMinWidth; window.MinHeight = normalMinHeight;
        // The longest source kind exercises the actual combo template and binding.
        showPage.Invoke(window, [0]); window.InputKindIndex = 1;
        await Capture(window, "extracted-folder-source-kind");
        var sourceKinds = (ComboBox)window.FindName("SourceKindBox");
        sourceKinds.IsDropDownOpen = true;
        await Settle(window);
        foreach (Popup popup in Descendants(sourceKinds).OfType<Popup>())
            if (popup.IsOpen && popup.Child is FrameworkElement child) await Capture(child, "source-kind-open-popup");
        sourceKinds.IsDropDownOpen = false;
        window.WindowState = WindowState.Maximized;
        await Capture(window, "maximized");
        window.WindowState = WindowState.Normal;
        await Capture(window, "restored");
        using var notices = new StreamReader(typeof(MainWindow).Assembly.GetManifestResourceStream("Builder.notices.txt")!);
        var about = new AboutWindow(window.VersionLabel, diagnostics, notices.ReadToEnd(), () => { }, () => Task.CompletedTask, () => Task.CompletedTask) { Owner = window };
        about.Show();
        await Capture(about, "about");
        about.Close();
        var colour = new ColourWindow("#12AAEF", diagnostics) { Owner = window };
        colour.Show();
        await Capture(colour, "colour");
        colour.Close();
    }

    private static async Task Settle(FrameworkElement element)
    {
        await element.Dispatcher.InvokeAsync(element.UpdateLayout, DispatcherPriority.Loaded);
        await element.Dispatcher.InvokeAsync(element.UpdateLayout, DispatcherPriority.Render);
        await element.Dispatcher.InvokeAsync(element.UpdateLayout, DispatcherPriority.ApplicationIdle);
    }

    private static async Task Capture(FrameworkElement root, string label)
    {
        await Settle(root);
        DpiScale dpi = VisualTreeHelper.GetDpi(root);
        string name = galleryMode ? label : $"{++serial:000}-{label}";
        var measurements = new List<object>();
        Rect viewport = new(new Point(), root.RenderSize);
        foreach (FrameworkElement control in Descendants(root).OfType<FrameworkElement>())
        {
            if (!control.IsVisible || control is not (TextBlock or TextBox or ComboBox or ButtonBase or Slider)) continue;
            if (control is TextBlock && HasEditableAncestor(control, root)) continue;
            Rect bounds = control.TransformToAncestor(root).TransformBounds(new Rect(new Point(), control.RenderSize));
            string text = control switch { TextBlock block => block.Text, TextBox box => box.Text,
                ContentControl content => content.Content?.ToString() ?? "", _ => AutomationProperties.GetName(control) };
            string identity = string.IsNullOrEmpty(control.Name) ? AutomationProperties.GetName(control) : control.Name;
            if (string.IsNullOrEmpty(identity)) identity = text.Length <= 120 ? text : text[..120];
            string? finding = null;
            if (control.ActualWidth < 1 || control.ActualHeight < 1) finding = "Visible semantic control has no usable layout size";
            // Vertical overflow is normal inside the page scroll view; horizontal clipping is not.
            bool verticallyOnScreen = bounds.Bottom > 0 && bounds.Top < viewport.Height;
            if (verticallyOnScreen && (bounds.Left < -1 || bounds.Right > viewport.Right + 1)) finding = "Control extends outside the visible window horizontally";
            if (control.Name == "NextButton" && !viewport.Contains(bounds)) finding = "Primary action is not fully visible";
            if (finding is not null) Issues.Add(new { severity = "error", capture = name, control = identity, finding, bounds = Box(bounds) });
            double? naturalTextHeight = null;
            if (control is TextBlock textBlock && !string.IsNullOrWhiteSpace(textBlock.Text) && textBlock.ActualWidth > 1 && textBlock.TextTrimming == TextTrimming.None)
            {
                // Measure text using the actual resolved typeface, size, wrapping width and DPI.
                // This is only a review flag: inline typography and nonstandard line layout may differ.
                var formatted = new FormattedText(textBlock.Text, CultureInfo.CurrentCulture, textBlock.FlowDirection,
                    new Typeface(textBlock.FontFamily, textBlock.FontStyle, textBlock.FontWeight, textBlock.FontStretch),
                    textBlock.FontSize, Brushes.White, dpi.PixelsPerDip);
                if (textBlock.TextWrapping != TextWrapping.NoWrap) formatted.MaxTextWidth = Math.Max(1, textBlock.ActualWidth - textBlock.Padding.Left - textBlock.Padding.Right);
                if (!double.IsNaN(textBlock.LineHeight)) formatted.LineHeight = textBlock.LineHeight;
                naturalTextHeight = formatted.Height + textBlock.Padding.Top + textBlock.Padding.Bottom;
                if (naturalTextHeight > textBlock.ActualHeight + 3)
                    Issues.Add(new { severity = "review", capture = name, control = identity,
                        finding = "Text metrics suggest insufficient vertical space; inspect capture", expected = naturalTextHeight, actual = textBlock.ActualHeight });
            }
            measurements.Add(new { type = control.GetType().Name, name = identity, text, control.IsEnabled,
                bounds = Box(bounds), desiredWidth = control.DesiredSize.Width, desiredHeight = control.DesiredSize.Height, naturalTextHeight });
        }
        int width = Math.Max(1, (int)Math.Ceiling(root.ActualWidth * dpi.DpiScaleX));
        int height = Math.Max(1, (int)Math.Ceiling(root.ActualHeight * dpi.DpiScaleY));
        var bitmap = new RenderTargetBitmap(width, height, dpi.PixelsPerInchX, dpi.PixelsPerInchY, PixelFormats.Pbgra32);
        bitmap.Render(root);
        var encoder = new PngBitmapEncoder(); encoder.Frames.Add(BitmapFrame.Create(bitmap));
        string path = Path.Combine(destination, name + ".png");
        using (FileStream file = File.Create(path)) encoder.Save(file);
        Captures.Add(new { name, path = Path.GetFileName(path), bytes = new FileInfo(path).Length,
            sha256 = Convert.ToHexStringLower(SHA256.HashData(File.ReadAllBytes(path))),
            dpiX = dpi.PixelsPerInchX, dpiY = dpi.PixelsPerInchY,
            logicalWidth = root.ActualWidth, logicalHeight = root.ActualHeight, pixelWidth = width, pixelHeight = height, measurements });
        Console.WriteLine($"Captured actual WPF view: {name}, {width}x{height}, {dpi.PixelsPerInchX:0} DPI");
    }

    private static bool HasEditableAncestor(DependencyObject element, DependencyObject root)
    {
        for (DependencyObject? parent = VisualTreeHelper.GetParent(element); parent is not null && parent != root; parent = VisualTreeHelper.GetParent(parent))
            if (parent is TextBox or ComboBox) return true;
        return false;
    }
    private static IEnumerable<DependencyObject> Descendants(DependencyObject root)
    {
        for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); i++)
        {
            DependencyObject child = VisualTreeHelper.GetChild(root, i);
            yield return child;
            foreach (DependencyObject descendant in Descendants(child)) yield return descendant;
        }
    }
    private static object Box(Rect rect) => new { x = rect.X, y = rect.Y, width = rect.Width, height = rect.Height };
    private static void WriteReport(string status, string note)
    {
        string assembly = typeof(MainWindow).Assembly.Location;
        var report = new { status, verifiedUtc = DateTimeOffset.UtcNow, windowsGuiExecution = Captures.Count == 0 ? "NOT_RUN" : "EXECUTED",
            framework = "WPF", mode = galleryMode ? "gallery" : "layout-audit", note,
            captureMethod = "RenderTargetBitmap of the production WPF MainWindow, compiled XAML/BAML, resources and bindings at the real monitor DPI.",
            frameGenerationUnavailableVerified = galleryMode ? (bool?)frameGenerationUnavailableVerified : null,
            physicalDpiMatrix = "Run separately at actual Windows 100%, 125%, 150%, 200%; the harness does not change display settings.",
            compactViewport = "The 608x480 logical case temporarily lowers this test window's minimum dimensions; it does not change monitor DPI or the production executable.",
            productionAssembly = assembly, productionSha256 = string.IsNullOrEmpty(assembly) ? null : Convert.ToHexStringLower(SHA256.HashData(File.ReadAllBytes(assembly))),
            captures = Captures, issues = Issues,
            limitations = new[] { "RenderTargetBitmap captures WPF client visuals; the DWM Acrylic backdrop and native shadows require desktop visual inspection.",
                "No game data, ISO extraction, file picker, PS5 execution or generated-frame behavior is exercised by this layout harness.",
                "Visible means present in the current WPF visual tree; scroll-view content may intentionally extend vertically outside the viewport." } };
        File.WriteAllText(Path.Combine(destination, "layout-report.json"), JsonSerializer.Serialize(report, JsonOptions));
    }

    private sealed class BindingListener : TraceListener
    {
        public List<string> Lines { get; } = [];
        public override void Write(string? message) { if (!string.IsNullOrWhiteSpace(message)) Lines.Add(message); }
        public override void WriteLine(string? message) => Write(message);
    }
}
