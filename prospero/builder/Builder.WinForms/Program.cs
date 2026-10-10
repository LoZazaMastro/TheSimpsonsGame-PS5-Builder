using System.Globalization;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;

namespace Simpsons.Builder.Desktop;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        using DiagnosticSession diagnostics = DiagnosticSession.Start();
        try
        {
            CultureInfo.DefaultThreadCurrentCulture = CultureInfo.GetCultureInfo("en-US");
            CultureInfo.DefaultThreadCurrentUICulture = CultureInfo.GetCultureInfo("en-US");
            Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.SetUnhandledExceptionMode(UnhandledExceptionMode.CatchException);
            Application.ThreadException += async (_, e) =>
            {
                diagnostics.Error("ui.unhandled-exception", e.Exception); await diagnostics.FlushAsync();
                MessageBox.Show("The builder encountered an unexpected problem.\n\n" + e.Exception.Message + "\n\nDiagnostics:\n" + diagnostics.LocationDescription,
                    "The Simpsons Game PS5 Builder", MessageBoxButtons.OK, MessageBoxIcon.Error);
            };
            AppDomain.CurrentDomain.UnhandledException += (_, e) =>
            {
                if (e.ExceptionObject is Exception exception) diagnostics.Error("process.unhandled-exception", exception, new { e.IsTerminating });
                else diagnostics.Record("process.unhandled-exception", new { e.IsTerminating, exception = e.ExceptionObject?.ToString() }, "error");
                try { diagnostics.FlushAsync().Wait(1500); } catch { }
            };
            TaskScheduler.UnobservedTaskException += (_, e) => diagnostics.Error("task.unobserved-exception", e.Exception);
            diagnostics.Record("bootstrap.resources.start");
            using Stream manifestStream = Resource("payload.json");
            using Stream settingsStream = Resource("capabilities.json");
            using var manifestBytes = new MemoryStream(); manifestStream.CopyTo(manifestBytes); manifestBytes.Position = 0;
            using var settingBytes = new MemoryStream(); settingsStream.CopyTo(settingBytes); settingBytes.Position = 0;
            var bundle = new BundlePackage(JsonFiles.Read<PayloadManifest>(manifestBytes), JsonFiles.Read<Capabilities>(settingBytes), () => Resource("payload.zip"));
            bundle.Validate();
            diagnostics.Record("bootstrap.resources.verified", new { bundle.Manifest.BuildVersion, bundle.Manifest.TitleId,
                manifestSha256 = Convert.ToHexString(SHA256.HashData(manifestBytes.ToArray())).ToLowerInvariant(),
                capabilitiesSha256 = Convert.ToHexString(SHA256.HashData(settingBytes.ToArray())).ToLowerInvariant(),
                payloadFiles = bundle.Manifest.Files, payloadBytes = bundle.Manifest.TotalBytes, settingsCount = bundle.Capabilities.Settings.Count });
            using var form = new MainForm(bundle, diagnostics);
            if (args.Length == 2 && args[0] == "--render-preview")
            {
                form.Shown += async (_, _) =>
                {
                    await Task.Delay(300);
                    form.SavePreview(args[1]);
                    form.Close();
                };
            }
            Application.Run(form);
            diagnostics.Record("application.exit", new { exitCode = 0 });
            return 0;
        }
        catch (Exception e)
        {
            diagnostics.Error("bootstrap.failed", e); diagnostics.FlushAsync().GetAwaiter().GetResult();
            if (args.Length == 2 && args[0] == "--render-preview")
            { try { File.WriteAllText(args[1] + ".error.txt", e + "\nLog: " + diagnostics.LocationDescription); } catch (Exception saveError) { diagnostics.Error("preview.error-report.failed", saveError); } }
            MessageBox.Show("The builder could not start.\n\n" + e.Message + "\n\nDiagnostics:\n" + diagnostics.LocationDescription,
                "The Simpsons Game PS5 Builder", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
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
