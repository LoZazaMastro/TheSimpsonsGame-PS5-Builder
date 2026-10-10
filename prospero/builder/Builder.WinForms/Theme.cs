using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace Simpsons.Builder.Desktop;

internal static class Theme
{
    public static readonly Color Ink = Color.FromArgb(28, 41, 65);
    public static readonly Color Muted = Color.FromArgb(90, 110, 132);
    public static readonly Color Sky = Color.FromArgb(29, 147, 229);
    public static readonly Color Yellow = Color.FromArgb(255, 213, 33);
    public static readonly Color Page = Color.FromArgb(239, 247, 253);
    public static readonly Color Line = Color.FromArgb(217, 231, 242);
    public static readonly Color Good = Color.FromArgb(18, 116, 83);
    private static readonly PrivateFontCollection TitleFonts = new();
    private static IntPtr titleFontMemory;
    private static bool loaded;

    public static Font Body(float size = 11, FontStyle style = FontStyle.Regular) => new("Segoe UI", size, style);
    public static Font Title(float size)
    {
        if (!loaded)
        {
            loaded = true;
            using Stream stream = Program.Resource("title.ttf");
            using var memory = new MemoryStream();
            stream.CopyTo(memory);
            byte[] font = memory.ToArray();
            titleFontMemory = Marshal.AllocCoTaskMem(font.Length);
            Marshal.Copy(font, 0, titleFontMemory, font.Length);
            TitleFonts.AddMemoryFont(titleFontMemory, font.Length);
            uint count = 0;
            AddFontMemResourceEx(titleFontMemory, (uint)font.Length, IntPtr.Zero, ref count);
        }
        return new Font(TitleFonts.Families[0], size);
    }

    [DllImport("gdi32.dll")]
    private static extern IntPtr AddFontMemResourceEx(IntPtr data, uint size, IntPtr reserved, ref uint count);

    public static GraphicsPath Rounded(RectangleF bounds, float radius)
    {
        float d = Math.Min(radius * 2, Math.Min(bounds.Width, bounds.Height));
        var path = new GraphicsPath();
        path.AddArc(bounds.X, bounds.Y, d, d, 180, 90);
        path.AddArc(bounds.Right - d, bounds.Y, d, d, 270, 90);
        path.AddArc(bounds.Right - d, bounds.Bottom - d, d, d, 0, 90);
        path.AddArc(bounds.X, bounds.Bottom - d, d, d, 90, 90);
        path.CloseFigure();
        return path;
    }

    public static Label Label(string text, float size = 11, bool bold = false, Color? color = null) => new()
    {
        Text = text, AutoSize = true, Font = Body(size, bold ? FontStyle.Bold : FontStyle.Regular),
        ForeColor = color ?? Ink, BackColor = Color.Transparent, Margin = new Padding(0, 0, 0, 8)
    };
}

internal sealed class ActionButton : Button
{
    [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
    public bool Primary { get; set; }
    public ActionButton(string text, bool primary = false)
    {
        Text = text; Primary = primary; FlatStyle = FlatStyle.Flat; FlatAppearance.BorderSize = 0;
        Font = Theme.Body(11, FontStyle.Bold); Height = 45; Width = 150; Cursor = Cursors.Hand;
        BackColor = primary ? Theme.Yellow : Color.White; ForeColor = Theme.Ink;
        Margin = new Padding(5); UseVisualStyleBackColor = false;
    }
    protected override void OnPaint(PaintEventArgs e)
    {
        float scale = DeviceDpi / 96f;
        e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
        using GraphicsPath path = Theme.Rounded(new RectangleF(scale, scale, Width - 3 * scale, Height - 3 * scale), 10 * scale);
        using var brush = new SolidBrush(Enabled ? (Primary ? Theme.Yellow : Color.White) : Theme.Line);
        e.Graphics.FillPath(brush, path);
        using var border = new Pen(Primary ? Color.FromArgb(226, 177, 0) : Theme.Line, scale);
        e.Graphics.DrawPath(border, path);
        TextRenderer.DrawText(e.Graphics, Text, Font, ClientRectangle, Enabled ? Theme.Ink : Theme.Muted,
            TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.EndEllipsis);
        if (Focused) ControlPaint.DrawFocusRectangle(e.Graphics, Rectangle.Inflate(ClientRectangle, -(int)(6 * scale), -(int)(6 * scale)));
    }
}

internal sealed class Card : UserControl
{
    public Card()
    {
        SuspendLayout();
        AutoScaleMode = AutoScaleMode.Dpi; AutoScaleDimensions = new SizeF(96, 96);
        DoubleBuffered = true; BackColor = Color.White; Padding = new Padding(22);
        Margin = new Padding(0, 0, 0, 16); AutoSize = true; AutoSizeMode = AutoSizeMode.GrowAndShrink;
    }
    protected override void OnPaint(PaintEventArgs e)
    {
        base.OnPaint(e);
        float scale = DeviceDpi / 96f;
        using var pen = new Pen(Theme.Line, scale);
        using GraphicsPath path = Theme.Rounded(new RectangleF(0, 0, Width - scale, Height - scale), 14 * scale);
        e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
        e.Graphics.DrawPath(pen, path);
    }
}

internal sealed class StepButton : Button
{
    public int Number { get; }
    public string Caption { get; }
    [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
    public bool Selected { get; set; }
    [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
    public bool Done { get; set; }
    public StepButton(int number, string title, string caption)
    {
        Number = number; Text = title; Caption = caption; Height = 72; Width = 225;
        FlatStyle = FlatStyle.Flat; FlatAppearance.BorderSize = 0; BackColor = Theme.Sky;
        Font = Theme.Body(11, FontStyle.Bold); ForeColor = Color.White; Cursor = Cursors.Hand;
        Margin = new Padding(0, 5, 0, 5); UseVisualStyleBackColor = false;
    }
    protected override void OnPaint(PaintEventArgs e)
    {
        float scale = DeviceDpi / 96f;
        Rectangle Box(int x, int y, int width, int height) => new((int)(x * scale), (int)(y * scale), (int)(width * scale), (int)(height * scale));
        e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
        if (Selected)
        {
            using GraphicsPath outline = Theme.Rounded(new RectangleF(0, 0, Width - scale, Height - scale), 12 * scale);
            using var selected = new SolidBrush(Color.FromArgb(36, 107, 184));
            e.Graphics.FillPath(selected, outline);
        }
        using var circle = new SolidBrush(Selected || Done ? Theme.Yellow : Color.FromArgb(106, 191, 246));
        e.Graphics.FillEllipse(circle, Box(12, 18, 35, 35));
        TextRenderer.DrawText(e.Graphics, Done && !Selected ? "✓" : Number.ToString(), Font,
            Box(12, 18, 35, 35), Theme.Ink, TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter);
        TextRenderer.DrawText(e.Graphics, Text, Font, new Rectangle((int)(58 * scale), (int)(15 * scale), Width - (int)(62 * scale), (int)(26 * scale)), Color.White,
            TextFormatFlags.EndEllipsis | TextFormatFlags.SingleLine);
        using Font detail = Theme.Body(8.7f);
        TextRenderer.DrawText(e.Graphics, Caption, detail, new Rectangle((int)(58 * scale), (int)(40 * scale), Width - (int)(62 * scale), (int)(24 * scale)), Color.FromArgb(221, 244, 255),
            TextFormatFlags.EndEllipsis | TextFormatFlags.SingleLine);
        if (Focused) ControlPaint.DrawFocusRectangle(e.Graphics, Rectangle.Inflate(ClientRectangle, -3, -3));
    }
}

internal sealed class SkyPanel : Panel
{
    public SkyPanel() { DoubleBuffered = true; BackColor = Theme.Sky; }
    protected override void OnPaint(PaintEventArgs e)
    {
        base.OnPaint(e);
        e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
        using var cloud = new SolidBrush(Color.FromArgb(110, 242, 252, 255));
        DrawCloud(e.Graphics, cloud, -35, 67, 1.0f);
        DrawCloud(e.Graphics, cloud, Width - 90, 179, 0.72f);
        DrawCloud(e.Graphics, cloud, -42, Height - 110, 1.15f);
    }
    private static void DrawCloud(Graphics g, Brush brush, float x, float y, float scale)
    {
        g.FillEllipse(brush, x, y + 20 * scale, 120 * scale, 30 * scale);
        g.FillEllipse(brush, x + 18 * scale, y + 3 * scale, 48 * scale, 48 * scale);
        g.FillEllipse(brush, x + 52 * scale, y, 55 * scale, 48 * scale);
    }
}
