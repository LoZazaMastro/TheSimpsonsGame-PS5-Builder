using System.Windows;

namespace Simpsons.Builder.Desktop;

public partial class AboutWindow : Window
{
    private readonly DiagnosticSession diagnostics;
    private readonly Action openLogs;
    private readonly Func<Task> copy;
    private readonly Func<Task> export;
    private readonly IDisposable appearance;
    public string VersionLabel { get; }
    public string LogLocation { get; }
    public string Notices { get; }
    public AboutWindow(string version, DiagnosticSession diagnostics, string notices, Action onOpenLogs, Func<Task> onCopy, Func<Task> onExport)
    {
        this.diagnostics = diagnostics;
        VersionLabel = version; LogLocation = "Session log: " + diagnostics.LocationDescription; Notices = notices;
        openLogs = onOpenLogs; copy = onCopy; export = onExport;
        InitializeComponent(); DataContext = this; appearance = WindowAppearance.Attach(this, diagnostics);
        Closed += (_, _) => appearance.Dispose();
    }
    private void Close_Click(object sender, RoutedEventArgs e) => Close();
    private void Donate_Click(object sender, RoutedEventArgs e) => ProjectLinks.OpenDonation(this, diagnostics);
    private void OpenLogs_Click(object sender, RoutedEventArgs e) => openLogs();
    private async void Copy_Click(object sender, RoutedEventArgs e) => await copy();
    private async void Export_Click(object sender, RoutedEventArgs e) => await export();
}
