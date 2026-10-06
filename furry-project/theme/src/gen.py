#!/usr/bin/env python3
"""Generiert das finn-term rEFInd-Theme: Terminal-Style in Schwarz/Lila/Pink.
Regenerates background.png, selection_*.png and the font bitmap into the repo root."""
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import os

OUT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ======================================================================
#  CONFIG: edit this block, then run  python src/gen.py
# ======================================================================
W, H = 2560, 1440               # your display's native resolution
                                # (keep theme.conf's `resolution` line in sync)

FONT_PATH = "/usr/share/fonts/TTF/JetBrainsMonoNerdFont-Regular.ttf"
FONT_BOLD = "/usr/share/fonts/TTF/JetBrainsMonoNerdFont-Bold.ttf"

HOSTNAME = "faearch"            # shown pink in the top-left header

# fake boot log: (name, value, status), put your own hardware here
BOOT_ROWS = [
    ("cpu    ", "AMD RYZEN 7 9850X3D ", "[ ONLINE ]"),
    ("gpu    ", "RADEON RX 9070 XT ", "[ ONLINE ]"),
    ("memory ", "32768M DDR5 @ 5600MT/S ", "[ OK ]"),
    ("disk   ", "SAMSUNG 990 PRO 2TB ", "[ MOUNTED ]"),
    ("windows", "", "[ NOT FOUND ] :3"),
]

# Palette
BG      = (7, 4, 15)        # tiefschwarz mit Lila-Stich
PURPLE  = (167, 139, 250)   # Haupt-Lila (Text)
PURPLE_D= (109, 82, 190)    # dunkleres Lila (dots/divider)
DIM     = (74, 60, 120)     # gedimmt
PINK    = (244, 114, 182)   # Akzent-Pink
PINK_HI = (255, 158, 210)   # helles Pink
# ======================================================================

def scanlines(img, period=4, strength=70):
    """Dunkelt jede period-te Zeile ab (CRT-Look)."""
    px = img.load()
    for y in range(0, img.height, period):
        for x in range(img.width):
            p = px[x, y]
            if len(p) == 4:
                r, g, b, a = p
                px[x, y] = (r * (255 - strength) // 255,
                            g * (255 - strength) // 255,
                            b * (255 - strength) // 255, a)
            else:
                r, g, b = p
                px[x, y] = (r * (255 - strength) // 255,
                            g * (255 - strength) // 255,
                            b * (255 - strength) // 255)
    return img

# ---------------------------------------------------------------- background
def make_background():
    img = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(img)

    # sehr subtiler radialer Glow hinter der Icon-Zone (Mitte)
    glow = Image.new("L", (W, H), 0)
    gd = ImageDraw.Draw(glow)
    gd.ellipse([W//2 - 700, H//2 - 420, W//2 + 700, H//2 + 420], fill=26)
    glow = glow.filter(ImageFilter.GaussianBlur(180))
    tint = Image.new("RGB", (W, H), (60, 36, 110))
    img = Image.composite(tint, img, glow.point(lambda v: v))
    img = Image.blend(Image.new("RGB", (W, H), BG), img, 0.9)

    # Text-Layer (fürs Glühen separat)
    txt = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    td = ImageDraw.Draw(txt)
    f_hdr = ImageFont.truetype(FONT_BOLD, 34)
    f     = ImageFont.truetype(FONT_PATH, 26)

    x0, y = 90, 70
    lh = 44

    td.text((x0, y), HOSTNAME, font=f_hdr, fill=PINK_HI)
    wpx = td.textlength(HOSTNAME, font=f_hdr)
    td.text((x0 + wpx, y), " // boot sequence", font=f_hdr, fill=PURPLE)
    y += int(lh * 1.35)
    td.text((x0, y), "─" * 52, font=f, fill=PURPLE_D)
    y += lh

    dots_end_px = 1050  # rechte Kante der Punktreihe
    for name, val, status in BOOT_ROWS:
        td.text((x0, y), "> ", font=f, fill=PINK)
        x = x0 + td.textlength("> ", font=f)
        td.text((x, y), name + " :: ", font=f, fill=DIM)
        x += td.textlength(name + " :: ", font=f)
        td.text((x, y), val, font=f, fill=PURPLE)
        x += td.textlength(val, font=f)
        ndots = max(2, int((dots_end_px - x) // td.textlength(".", font=f)))
        td.text((x, y), "." * ndots + " ", font=f, fill=PURPLE_D)
        x += td.textlength("." * ndots + " ", font=f)
        td.text((x, y), status, font=f, fill=PINK if "NOT FOUND" in status or "ONLINE" in status else PURPLE)
        y += lh

    y += int(lh * 0.6)
    td.text((x0, y), "> awaiting boot target ...", font=f, fill=PURPLE)
    y += lh
    td.text((x0, y), "> ", font=f, fill=PINK)
    cx = x0 + td.textlength("> ", font=f)
    td.rectangle([cx, y + 4, cx + 16, y + 32], fill=PINK)  # Block-Cursor

    # Glow unter den Text legen
    glow_txt = txt.filter(ImageFilter.GaussianBlur(6))
    img = img.convert("RGBA")
    img.alpha_composite(glow_txt)
    img.alpha_composite(glow_txt)  # doppelt = kräftigeres Glühen
    img.alpha_composite(txt)
    img = img.convert("RGB")

    scanlines(img, period=4, strength=60)

    # Vignette
    vig = Image.new("L", (W, H), 0)
    vd = ImageDraw.Draw(vig)
    vd.rectangle([0, 0, W, H], fill=64)
    vd.ellipse([-300, -200, W + 300, H + 200], fill=0)
    vig = vig.filter(ImageFilter.GaussianBlur(250))
    img = Image.composite(Image.new("RGB", (W, H), (0, 0, 0)), img, vig)

    img.save(os.path.join(OUT, "background.png"))
    print("background.png ok")

# ---------------------------------------------------------------- selection
def make_selection(size, pad, name, lw=3):
    """Terminal-Eckklammern in Pink mit Glow + Scanline-Aussparung."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    c = pad          # Abstand vom Rand
    ln = size // 5   # Länge der Eck-Arme
    col = PINK + (255,)
    for (x, y, dx, dy) in [(c, c, 1, 1), (size - c, c, -1, 1),
                           (c, size - c, 1, -1), (size - c, size - c, -1, -1)]:
        d.line([x, y, x + dx * ln, y], fill=col, width=lw)
        d.line([x, y, x, y + dy * ln], fill=col, width=lw)
    glow = img.filter(ImageFilter.GaussianBlur(5))
    out = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    out.alpha_composite(glow)
    out.alpha_composite(glow)
    out.alpha_composite(img)
    scanlines(out, period=4, strength=90)
    out.save(os.path.join(OUT, name))
    print(name, "ok")

# ---------------------------------------------------------------- font
def make_font(size=30, name=None):
    """rEFInd-Font: 96 Zellen (ASCII 32-126 + Fallback '?') in einer Reihe."""
    f = ImageFont.truetype(FONT_PATH, size)
    cell = int(round(f.getlength("M")))
    h = size + 6
    img = Image.new("RGBA", (cell * 96, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    asc, desc = f.getmetrics()
    ybase = (h - (asc + desc)) // 2
    lavender = (196, 181, 253, 255)  # helles Lavendel für gute Lesbarkeit
    for i in range(95):
        ch = chr(32 + i)
        wpx = f.getlength(ch)
        d.text((i * cell + (cell - wpx) / 2, ybase), ch, font=f, fill=lavender)
    d.text((95 * cell + (cell - f.getlength("?")) / 2, ybase), "?", font=f, fill=lavender)
    fname = name or f"jetbrains-mono-{size}.png"
    img.save(os.path.join(OUT, "fonts", fname))
    print(fname, f"ok ({cell*96}x{h}, cell {cell}px)")

if __name__ == "__main__":
    os.makedirs(os.path.join(OUT, "fonts"), exist_ok=True)
    make_background()
    make_selection(288, 10, "selection_big.png", lw=4)
    make_selection(128, 6, "selection_small.png", lw=3)
    make_font(30)
