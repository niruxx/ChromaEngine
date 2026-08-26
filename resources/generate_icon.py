"""Generates the app icon (resources/app_icon.ico) offline. Not part of the
build - run manually if the design needs to change; the .ico is what's
actually embedded via platform/windows/app.rc."""

from PIL import Image, ImageDraw

SIZE = 256

# Three-stop diagonal "chroma" gradient (pink -> violet -> cyan) for
# ChromaEngine, replacing the old two-stop purple/blue Colorfy Engine mark.
STOPS = [
    (0xFF, 0x4D, 0x6D),
    (0x9B, 0x30, 0xFF),
    (0x00, 0xC9, 0xFF),
]


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def gradient_color(t: float):
    t = max(0.0, min(1.0, t))
    segment = t * (len(STOPS) - 1)
    i = min(int(segment), len(STOPS) - 2)
    local_t = segment - i
    return lerp(STOPS[i], STOPS[i + 1], local_t)


def make_base(size: int) -> Image.Image:
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    radius = int(size * 0.22)

    gradient = Image.new("RGBA", (size, size), 0)
    gdraw = ImageDraw.Draw(gradient)
    diag = (size - 1) * 2
    for y in range(size):
        row_colors = []
        for x in range(size):
            t = (x + y) / diag
            row_colors.append(gradient_color(t))
        for x in range(size):
            gdraw.point((x, y), fill=row_colors[x] + (255,))

    mask = Image.new("L", (size, size), 0)
    mdraw = ImageDraw.Draw(mask)
    mdraw.rounded_rectangle([0, 0, size - 1, size - 1], radius=radius, fill=255)

    img.paste(gradient, (0, 0), mask)

    tri = [
        (size * 0.40, size * 0.28),
        (size * 0.40, size * 0.72),
        (size * 0.76, size * 0.50),
    ]
    draw.polygon(tri, fill=(255, 255, 255, 235))

    return img


base = make_base(SIZE)
sizes = [16, 32, 48, 64, 128, 256]
base.save(
    "resources/app_icon.ico",
    sizes=[(s, s) for s in sizes],
)
print("wrote resources/app_icon.ico")
