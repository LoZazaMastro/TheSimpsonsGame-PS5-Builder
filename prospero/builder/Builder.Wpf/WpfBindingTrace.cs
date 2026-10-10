using System.Diagnostics;
using System.Text;

namespace Simpsons.Builder.Desktop;

internal sealed class WpfBindingTrace : TraceListener
{
    private readonly DiagnosticSession diagnostics;
    private readonly StringBuilder line = new();
    private readonly object gate = new();
    private readonly SourceLevels previousLevel;
    private bool disposed;

    internal WpfBindingTrace(DiagnosticSession diagnostics)
    {
        this.diagnostics = diagnostics;
        previousLevel = PresentationTraceSources.DataBindingSource.Switch.Level;
        PresentationTraceSources.DataBindingSource.Listeners.Add(this);
        PresentationTraceSources.DataBindingSource.Switch.Level = SourceLevels.Warning;
    }

    public override void Write(string? message) { lock (gate) line.Append(message); }
    public override void WriteLine(string? message)
    {
        lock (gate)
        {
            line.Append(message);
            diagnostics.Record("ui.binding-warning", new { message = line.ToString() }, "warning");
            line.Clear();
        }
    }
    protected override void Dispose(bool disposing)
    {
        if (disposing && !disposed)
        {
            disposed = true;
            PresentationTraceSources.DataBindingSource.Listeners.Remove(this);
            PresentationTraceSources.DataBindingSource.Switch.Level = previousLevel;
        }
        base.Dispose(disposing);
    }
}
