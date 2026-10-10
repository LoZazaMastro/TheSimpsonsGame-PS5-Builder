"""Render the project's original folder/arrow vector mark as a Windows icon."""
from pathlib import Path
from PIL import Image, ImageDraw


def main():
    root = Path(__file__).resolve().parent
    scale = 4
    image = Image.new("RGBA", (256 * scale, 256 * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((0, 0, 256 * scale - 1, 256 * scale - 1), radius=52 * scale, fill="#17191b")

    def line(points):
        xy = [(x * scale, y * scale) for x, y in points]
        draw.line(xy, fill="#ffcb0f", width=14 * scale, joint="curve")
        for x, y in xy:
            r = 7 * scale
            draw.ellipse((x - r, y - r, x + r, y + r), fill="#ffcb0f")

    line([(46, 85), (103, 85), (121, 104), (209, 104), (209, 201), (46, 201), (46, 85)])
    line([(46, 85), (46, 59), (104, 59), (122, 79), (191, 79), (191, 104)])
    line([(128, 132), (128, 171)])
    line([(109, 153), (128, 172), (147, 153)])
    image = image.resize((256, 256), Image.Resampling.LANCZOS)
    image.save(root / "Assets/builder.ico", format="ICO", sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])


if __name__ == "__main__":
    main()
