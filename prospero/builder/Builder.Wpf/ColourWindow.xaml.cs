using System.ComponentModel;
using System.Windows;
using System.Windows.Media;

namespace Simpsons.Builder.Desktop;

public partial class ColourWindow : Window, INotifyPropertyChanged
{
    private double red, green, blue;
    private readonly IDisposable appearance;
    public event PropertyChangedEventHandler? PropertyChanged;
    public double Red { get => red; set { if (red != value) { red = Math.Clamp(Math.Round(value), 0, 255); Notify(); } } }
    public double Green { get => green; set { if (green != value) { green = Math.Clamp(Math.Round(value), 0, 255); Notify(); } } }
    public double Blue { get => blue; set { if (blue != value) { blue = Math.Clamp(Math.Round(value), 0, 255); Notify(); } } }
    public string SelectedHex => $"#{(byte)red:X2}{(byte)green:X2}{(byte)blue:X2}";
    public Brush PreviewBrush => new SolidColorBrush(Color.FromRgb((byte)red, (byte)green, (byte)blue));
    public ColourWindow(string hex, DiagnosticSession diagnostics)
    {
        Color color = (Color)ColorConverter.ConvertFromString(hex)!;
        red = color.R; green = color.G; blue = color.B;
        InitializeComponent(); DataContext = this;
        appearance = WindowAppearance.Attach(this, diagnostics); Closed += (_, _) => appearance.Dispose();
    }
    private void Notify()
    { foreach (string name in new[] { nameof(Red), nameof(Green), nameof(Blue), nameof(SelectedHex), nameof(PreviewBrush) }) PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name)); }
    private void Close_Click(object sender, RoutedEventArgs e) => Close();
    private void Accept_Click(object sender, RoutedEventArgs e) { DialogResult = true; }
}
