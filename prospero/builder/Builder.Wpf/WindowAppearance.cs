using System.ComponentModel;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Shell;
using System.Windows.Threading;
using Microsoft.Win32;
using Simpsons.Builder;

namespace Simpsons.Builder.Desktop;

/// <summary>
/// Native window appearance only. WindowChrome retains resize hit-testing, title-bar
/// dragging, double-click maximize, the system menu and Windows' normal DPI processing.
/// Documented Desktop Acrylic is available on Windows 11 build 22621 and later.
/// https://learn.microsoft.com/windows/win32/api/dwmapi/ne-dwmapi-dwm_systembackdrop_type
/// https://learn.microsoft.com/dotnet/api/system.windows.shell.windowchrome
/// </summary>
internal sealed class WindowAppearance : IDisposable
{
    private const int WmGetMinMaxInfo = 0x0024;
    private const int WmSettingChange = 0x001A;
    private const int WmDisplayChange = 0x007E;
    private const int WmThemeChanged = 0x031A;
    private const int WmDwmCompositionChanged = 0x031E;
    private const int WmDpiChanged = 0x02E0;
    private const int DwmUseImmersiveDarkMode = 20;
    private const int DwmWindowCornerPreference = 33;
    private const int DwmBorderColor = 34;
    private const int DwmSystemBackdropType = 38;
    private const int BackdropNone = 1;
    private const int BackdropDesktopAcrylic = 3;
    private const uint MonitorDefaultToNearest = 2;
    private static readonly ConditionalWeakTable<Window, WindowAppearance> Attachments = new();
    private static readonly Color OpaqueColor = Color.FromRgb(0x17, 0x19, 0x1B);
    private static readonly Brush OpaqueBrush = CreateBrush(OpaqueColor);
    private static readonly Brush AcrylicTint = CreateBrush(Color.FromArgb(0xC4, 0x17, 0x19, 0x1B));

    private readonly Window window;
    private readonly DiagnosticSession diagnostics;
    private HwndSource? source;
    private nint handle;
    private bool disposed;
    private bool refreshQueued;
    private bool initialFitDone;
    private string? lastMode;
    private WindowChrome? chrome;

    private WindowAppearance(Window window, DiagnosticSession diagnostics)
    {
        this.window = window;
        this.diagnostics = diagnostics;
        window.SourceInitialized += OnSourceInitialized;
        window.Loaded += OnLoaded;
        window.StateChanged += OnStateChanged;
        window.Closed += OnClosed;
        SystemParameters.StaticPropertyChanged += OnSystemParametersChanged;
        if (new WindowInteropHelper(window).Handle != nint.Zero) InitializeSource();
    }

    public static IDisposable Attach(Window window, DiagnosticSession diagnostics)
    {
        ArgumentNullException.ThrowIfNull(window);
        ArgumentNullException.ThrowIfNull(diagnostics);
        window.Dispatcher.VerifyAccess();
        return Attachments.GetValue(window, key => new WindowAppearance(key, diagnostics));
    }

    private void OnSourceInitialized(object? sender, EventArgs e) => InitializeSource();
    private void OnLoaded(object sender, RoutedEventArgs e) => QueueRefresh();

    private void InitializeSource()
    {
        if (disposed || source is not null) return;
        handle = new WindowInteropHelper(window).Handle;
        source = HwndSource.FromHwnd(handle);
        if (handle == nint.Zero || source is null) return;
        source.AddHook(WindowProcedure);
        chrome = WindowChrome.GetWindowChrome(window);
        if (chrome?.IsFrozen == true)
        {
            chrome = (WindowChrome)chrome.Clone();
            WindowChrome.SetWindowChrome(window, chrome);
        }
        FitInitialWindow();
        QueueRefresh();
    }

    private void FitInitialWindow()
    {
        if (initialFitDone || !TryGetMonitor(out var monitor)) return;
        initialFitDone = true;
        var scale = GetScale();
        // MONITORINFO is in physical pixels. Convert sizes exactly once to DIPs;
        // WPF continues to own CenterScreen and all subsequent WM_DPICHANGED moves.
        double width = Math.Max(1, (monitor.Work.Right - monitor.Work.Left) / scale - 32);
        double height = Math.Max(1, (monitor.Work.Bottom - monitor.Work.Top) / scale - 32);
        window.MinWidth = Math.Min(window.MinWidth, width);
        window.MinHeight = Math.Min(window.MinHeight, height);
        if (double.IsFinite(window.Width)) window.Width = Math.Min(window.Width, width);
        if (double.IsFinite(window.Height)) window.Height = Math.Min(window.Height, height);
        diagnostics.Record("window.initial_layout", new
        {
            dpi = (int)Math.Round(scale * 96),
            workWidthDip = Math.Round(width + 32, 1),
            workHeightDip = Math.Round(height + 32, 1),
            width = window.Width,
            height = window.Height,
            minWidth = window.MinWidth,
            minHeight = window.MinHeight
        });
    }

    private void OnStateChanged(object? sender, EventArgs e)
    {
        diagnostics.Record("window.state", new { state = window.WindowState.ToString() });
        QueueRefresh();
    }

    private void OnSystemParametersChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName is nameof(SystemParameters.HighContrast) or nameof(SystemParameters.ClientAreaAnimation))
            QueueRefresh();
    }

    private void QueueRefresh()
    {
        if (disposed || refreshQueued || window.Dispatcher.HasShutdownStarted) return;
        refreshQueued = true;
        _ = window.Dispatcher.BeginInvoke(DispatcherPriority.Loaded, (Action)(() =>
        {
            refreshQueued = false;
            if (!disposed) ApplyAppearance();
        }));
    }

    private void ApplyAppearance()
    {
        if (disposed || handle == nint.Zero || source?.CompositionTarget is null) return;
        bool highContrast = SystemParameters.HighContrast;
        bool transparencyEnabled = ReadTransparencyPreference();
        bool acrylic = false;
        string reason;
        int? result = null;
        int? frameResult = null;
        try
        {
            if (OperatingSystem.IsWindowsVersionAtLeast(10, 0, 22000))
            {
                SetAttribute(DwmUseImmersiveDarkMode, 1);
                SetAttribute(DwmWindowCornerPreference, window.WindowState == WindowState.Maximized ? 1 : 2);
                // COLORREF is 0x00BBGGRR, not an ARGB/XAML color.
                SetAttribute(DwmBorderColor, 0x00302D2A);
            }
            if (!OperatingSystem.IsWindowsVersionAtLeast(10, 0, 22621)) reason = "windows_before_22621";
            else if (highContrast) reason = "high_contrast";
            else if (!transparencyEnabled) reason = "transparency_disabled";
            else if (DwmIsCompositionEnabled(out bool compositionEnabled) < 0 || !compositionEnabled)
                reason = "composition_unavailable";
            else
            {
                result = SetAttribute(DwmSystemBackdropType, BackdropDesktopAcrylic);
                if (result >= 0)
                {
                    // A zero GlassFrameThickness makes WindowChrome install a GDI region.
                    // Full glass clears that region, allowing the native DWM round corners.
                    SetChromeGlass(true);
                    var margins = new Margins(-1);
                    frameResult = DwmExtendFrameIntoClientArea(handle, ref margins);
                    acrylic = frameResult >= 0;
                }
                reason = acrylic ? "desktop_acrylic" : "native_backdrop_unavailable";
            }

            if (!acrylic)
            {
                if (OperatingSystem.IsWindowsVersionAtLeast(10, 0, 22621))
                    SetAttribute(DwmSystemBackdropType, BackdropNone);
                SetChromeGlass(false);
                var margins = new Margins(0);
                DwmExtendFrameIntoClientArea(handle, ref margins);
            }
        }
        catch (Exception error) when (error is DllNotFoundException or EntryPointNotFoundException or ExternalException)
        {
            reason = "native_api_unavailable";
            if (lastMode != reason) diagnostics.Error("window.appearance_fallback", error);
            SetChromeGlass(false);
        }

        source.CompositionTarget.BackgroundColor = acrylic ? Colors.Transparent : OpaqueColor;
        window.Background = Brushes.Transparent;
        window.Resources["WindowBackdrop"] = acrylic ? AcrylicTint : OpaqueBrush;
        if (window.FindName("WindowSurface") is Border surface)
            surface.CornerRadius = new CornerRadius(window.WindowState == WindowState.Maximized ? 0 : 8);

        if (lastMode != reason)
        {
            lastMode = reason;
            diagnostics.Record("window.appearance", new
            {
                mode = reason,
                acrylic,
                highContrast,
                transparencyEnabled,
                backdropHresult = result.HasValue ? $"0x{result.Value:X8}" : null,
                frameHresult = frameResult.HasValue ? $"0x{frameResult.Value:X8}" : null,
                osBuild = Environment.OSVersion.Version.Build
            });
        }
    }

    private void SetChromeGlass(bool enabled)
    {
        if (chrome is null) return;
        var thickness = enabled ? new Thickness(-1) : new Thickness(0);
        if (chrome.GlassFrameThickness != thickness) chrome.GlassFrameThickness = thickness;
        chrome.UseAeroCaptionButtons = false;
        // Older Windows can still round the fallback using WindowChrome's GDI region.
        var radius = new CornerRadius(window.WindowState == WindowState.Maximized ? 0 : 8);
        if (chrome.CornerRadius != radius) chrome.CornerRadius = radius;
    }

    private int SetAttribute(int attribute, int value) => DwmSetWindowAttribute(handle, attribute, ref value, sizeof(int));

    private bool ReadTransparencyPreference()
    {
        try
        {
            using RegistryKey? key = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Themes\Personalize");
            return key?.GetValue("EnableTransparency") is not int value || value != 0;
        }
        catch (Exception error) when (error is SecurityException or UnauthorizedAccessException or IOException)
        {
            // Respect an unreadable policy by choosing the always-readable opaque surface.
            if (lastMode != "transparency_disabled") diagnostics.Error("window.transparency_preference_unavailable", error);
            return false;
        }
    }

    private nint WindowProcedure(nint hwnd, int message, nint wParam, nint lParam, ref bool handled)
    {
        if (disposed) return nint.Zero;
        if (message == WmGetMinMaxInfo && lParam != nint.Zero && TryGetMonitor(out var monitor))
        {
            // The frameless, full-glass window must maximize inside the current monitor's
            // work area. These values are already physical pixels, including negative monitor
            // origins; never multiply the work rectangle by a WPF DPI scale again.
            var limits = Marshal.PtrToStructure<MinMaxInfo>(lParam);
            limits.MaxPosition.X = monitor.Work.Left - monitor.Monitor.Left;
            limits.MaxPosition.Y = monitor.Work.Top - monitor.Monitor.Top;
            limits.MaxSize.X = monitor.Work.Right - monitor.Work.Left;
            limits.MaxSize.Y = monitor.Work.Bottom - monitor.Work.Top;
            double scale = GetScale();
            limits.MinTrackSize.X = Math.Max(limits.MinTrackSize.X, ToPixels(window.MinWidth, scale));
            limits.MinTrackSize.Y = Math.Max(limits.MinTrackSize.Y, ToPixels(window.MinHeight, scale));
            if (double.IsFinite(window.MaxWidth))
            {
                int maximum = ToPixels(window.MaxWidth, scale);
                limits.MaxTrackSize.X = Math.Min(limits.MaxTrackSize.X, maximum);
                limits.MaxSize.X = Math.Min(limits.MaxSize.X, maximum);
            }
            if (double.IsFinite(window.MaxHeight))
            {
                int maximum = ToPixels(window.MaxHeight, scale);
                limits.MaxTrackSize.Y = Math.Min(limits.MaxTrackSize.Y, maximum);
                limits.MaxSize.Y = Math.Min(limits.MaxSize.Y, maximum);
            }
            Marshal.StructureToPtr(limits, lParam, false);
            handled = true;
        }
        else if (message is WmSettingChange or WmDisplayChange or WmThemeChanged or WmDwmCompositionChanged or WmDpiChanged)
        {
            // WPF must receive the original messages, particularly WM_DPICHANGED.
            QueueRefresh();
        }
        return nint.Zero;
    }

    private bool TryGetMonitor(out MonitorInfo monitor)
    {
        monitor = new MonitorInfo { Size = Marshal.SizeOf<MonitorInfo>() };
        return handle != nint.Zero && GetMonitorInfo(MonitorFromWindow(handle, MonitorDefaultToNearest), ref monitor);
    }

    private double GetScale()
    {
        uint dpi = GetDpiForWindow(handle);
        return dpi > 0 ? dpi / 96d : VisualTreeHelper.GetDpi(window).DpiScaleX;
    }

    private static int ToPixels(double dip, double scale) => (int)Math.Clamp(Math.Ceiling(dip * scale), 0, int.MaxValue);

    private static Brush CreateBrush(Color color)
    {
        var brush = new SolidColorBrush(color);
        brush.Freeze();
        return brush;
    }

    private void OnClosed(object? sender, EventArgs e) => Dispose();

    public void Dispose()
    {
        if (disposed) return;
        disposed = true;
        window.SourceInitialized -= OnSourceInitialized;
        window.Loaded -= OnLoaded;
        window.StateChanged -= OnStateChanged;
        window.Closed -= OnClosed;
        SystemParameters.StaticPropertyChanged -= OnSystemParametersChanged;
        if (source is { IsDisposed: false }) source.RemoveHook(WindowProcedure);
        Attachments.Remove(window);
        source = null;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct Margins
    {
        public int Left, Right, Top, Bottom;
        public Margins(int value) => Left = Right = Top = Bottom = value;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct NativePoint { public int X, Y; }
    [StructLayout(LayoutKind.Sequential)]
    private struct NativeRect { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)]
    private struct MonitorInfo { public int Size; public NativeRect Monitor, Work; public uint Flags; }
    [StructLayout(LayoutKind.Sequential)]
    private struct MinMaxInfo { public NativePoint Reserved, MaxSize, MaxPosition, MinTrackSize, MaxTrackSize; }

    [DllImport("dwmapi.dll", PreserveSig = true)]
    private static extern int DwmSetWindowAttribute(nint hwnd, int attribute, ref int value, int size);
    [DllImport("dwmapi.dll", PreserveSig = true)]
    private static extern int DwmExtendFrameIntoClientArea(nint hwnd, ref Margins margins);
    [DllImport("dwmapi.dll", PreserveSig = true)]
    private static extern int DwmIsCompositionEnabled([MarshalAs(UnmanagedType.Bool)] out bool enabled);
    [DllImport("user32.dll")]
    private static extern nint MonitorFromWindow(nint hwnd, uint flags);
    [DllImport("user32.dll", EntryPoint = "GetMonitorInfoW", ExactSpelling = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetMonitorInfo(nint monitor, ref MonitorInfo info);
    [DllImport("user32.dll")]
    private static extern uint GetDpiForWindow(nint hwnd);
}
