using System.Globalization;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Windows;

namespace Simpsons.Builder.Desktop;

internal static class Program
{
    internal const string BuilderVersion = "3.10.6";

    [STAThread]
    private static int Main(string[] args)
    {
        using DiagnosticSession diagnostics = DiagnosticSession.Start();
        try
        {
            CultureInfo.DefaultThreadCurrentCulture = CultureInfo.GetCultureInfo("en-US");
            CultureInfo.DefaultThreadCurrentUICulture = CultureInfo.GetCultureInfo("en-US");
            diagnostics.Record("bootstrap.ui", new { builderVersion = BuilderVersion, framework = "WPF", theme = "dark-acrylic", displayScaling = "PerMonitorV2" });
            var app = new App();
            using var bindingTrace = new WpfBindingTrace(diagnostics);
            app.DispatcherUnhandledException += async (_, e) =>
            {
                diagnostics.Error("ui.unhandled-exception", e.Exception);
                e.Handled = true;
                await diagnostics.FlushAsync();
                MessageBox.Show("The builder encountered an unexpected problem.\n\n" + e.Exception.Message +
                    "\n\nDiagnostics:\n" + diagnostics.LocationDescription, "The Simpsons Game PS5 Builder", MessageBoxButton.OK, MessageBoxImage.Error);
            };
            AppDomain.CurrentDomain.UnhandledException += (_, e) =>
            {
                if (e.ExceptionObject is Exception exception) diagnostics.Error("process.unhandled-exception", exception, new { e.IsTerminating });
                else diagnostics.Record("process.unhandled-exception", new { e.IsTerminating, exception = e.ExceptionObject?.ToString() }, "error");
                try { diagnostics.FlushAsync().Wait(1500); } catch { }
            };
            TaskScheduler.UnobservedTaskException += (_, e) => diagnostics.Error("task.unobserved-exception", e.Exception);
            app.InitializeComponent();
            diagnostics.Record("bootstrap.resources.start");
            BundlePackage bundle = LoadBundle(diagnostics);
            var window = new MainWindow(bundle, diagnostics);
            if (args.Length == 2 && args[0] == "--render-preview")
            {
                bool previewStarted = false;
                window.ContentRendered += async (_, _) =>
                {
                    if (previewStarted) return;
                    previewStarted = true;
                    try { await window.SavePreviewAsync(args[1]); }
                    catch (Exception e)
                    {
                        diagnostics.Error("preview.failed", e);
                        try { File.WriteAllText(args[1] + ".error.txt", e.ToString()); } catch (Exception saveError) { diagnostics.Error("preview.error-report.failed", saveError); }
                    }
                    finally { window.Close(); }
                };
            }
            int exitCode = app.Run(window);
            diagnostics.Record("application.exit", new { exitCode });
            return exitCode;
        }
        catch (Exception e)
        {
            diagnostics.Error("bootstrap.failed", e);
            diagnostics.FlushAsync().GetAwaiter().GetResult();
            MessageBox.Show("The builder could not start.\n\n" + e.Message + "\n\nDiagnostics:\n" + diagnostics.LocationDescription,
                "The Simpsons Game PS5 Builder", MessageBoxButton.OK, MessageBoxImage.Error);
            return 1;
        }
    }

    internal static BundlePackage LoadBundle(DiagnosticSession diagnostics)
    {
        using Stream manifestStream = Resource("payload.json");
        using Stream settingsStream = Resource("capabilities.json");
        using var manifestBytes = new MemoryStream(); manifestStream.CopyTo(manifestBytes); manifestBytes.Position = 0;
        using var settingBytes = new MemoryStream(); settingsStream.CopyTo(settingBytes); settingBytes.Position = 0;
        var bundle = new BundlePackage(JsonFiles.Read<PayloadManifest>(manifestBytes), JsonFiles.Read<Capabilities>(settingBytes), () => Resource("payload.zip"));
        bundle.Validate();
        diagnostics.Record("bootstrap.resources.verified", new { bundle.Manifest.BuildVersion, bundle.Manifest.TitleId, bundle.Manifest.XboxTitleId,
            manifestSha256 = Convert.ToHexString(SHA256.HashData(manifestBytes.ToArray())).ToLowerInvariant(),
            capabilitiesSha256 = Convert.ToHexString(SHA256.HashData(settingBytes.ToArray())).ToLowerInvariant(),
            payloadFiles = bundle.Manifest.Files, payloadBytes = bundle.Manifest.TotalBytes, settingsCount = bundle.Capabilities.Settings.Count });
        return bundle;
    }

    internal static Stream Resource(string name) => Assembly.GetExecutingAssembly().GetManifestResourceStream("Builder." + name)
        ?? throw new BuilderException("A bundled file is missing: " + name);

    internal static string Notices()
    {
        using var reader = new StreamReader(Resource("notices.txt"), Encoding.UTF8);
        using var ps5 = new StreamReader(Resource("ps5-notices.txt"), Encoding.UTF8);
        return reader.ReadToEnd() + "\n\n" + ps5.ReadToEnd();
    }
}
