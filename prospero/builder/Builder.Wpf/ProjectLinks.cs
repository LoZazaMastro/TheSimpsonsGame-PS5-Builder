using System.Diagnostics;
using System.Windows;

namespace Simpsons.Builder.Desktop;

internal static class ProjectLinks
{
    internal const string DonationUrl = "https://ko-fi.com/lozazamastro";

    internal static void OpenDonation(Window owner, DiagnosticSession diagnostics)
    {
        try
        {
            Process.Start(new ProcessStartInfo(DonationUrl) { UseShellExecute = true });
            diagnostics.Record("ui.donate.opened", new { url = DonationUrl });
        }
        catch (Exception error)
        {
            diagnostics.Error("ui.donate.open-failed", error, new { url = DonationUrl });
            MessageBox.Show(owner, "The donation page could not be opened.\n\n" + DonationUrl +
                "\n\n" + error.Message + "\n\nDiagnostics:\n" + diagnostics.LocationDescription,
                "Donate", MessageBoxButton.OK, MessageBoxImage.Error);
        }
    }
}
