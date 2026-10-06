#!/usr/bin/env python3
"""Setzt eine realistische rEFInd-Vorschau aus den Theme-Assets zusammen."""
from PIL import Image, ImageDraw, ImageFont
import os

OUT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
bg = Image.open(os.path.join(OUT, "background.png")).convert("RGBA")
W, H = bg.size

sel_big = Image.open(os.path.join(OUT, "selection_big.png")).convert("RGBA")
arch = Image.open(os.path.join(OUT, "icons/os_arch.png")).convert("RGBA")
shell = Image.open(os.path.join(OUT, "icons/tool_shell.png")).convert("RGBA").resize((96, 96), Image.LANCZOS)
shutdown = Image.open(os.path.join(OUT, "icons/func_shutdown.png")).convert("RGBA").resize((96, 96), Image.LANCZOS)
reset = Image.open(os.path.join(OUT, "icons/func_reset.png")).convert("RGBA").resize((96, 96), Image.LANCZOS)

# rEFInd-Layout: großes Icon mittig, Selection drumherum
cy = int(H * 0.48)
sx = W // 2 - 144
bg.alpha_composite(sel_big, (sx, cy - 144))
bg.alpha_composite(arch, (W // 2 - 128, cy - 128))

# kleine Tool-Reihe darunter
ty = cy + 190
gap = 150
for i, ic in enumerate([shell, reset, shutdown]):
    bg.alpha_composite(ic, (W // 2 - (gap * 1) - 48 + i * gap, ty))

# Menütext aus dem generierten rEFInd-Font rendern (so sieht's wirklich aus)
fontimg = Image.open(os.path.join(OUT, "fonts/jetbrains-mono-30.png")).convert("RGBA")
cell = fontimg.width // 96
def draw_string(dst, s, cx_center, y):
    x = cx_center - len(s) * cell // 2
    for ch in s:
        idx = ord(ch) - 32
        if not (0 <= idx < 95):
            idx = 95
        g = fontimg.crop((idx * cell, 0, (idx + 1) * cell, fontimg.height))
        dst.alpha_composite(g, (x, y))
        x += cell

draw_string(bg, "Boot Arch Linux from FAEARCH", W // 2, ty + 130)
draw_string(bg, "Use arrow keys to move cursor; Enter to boot;", W // 2, H - 130)
draw_string(bg, "Insert, Tab, or F2 for more options; Esc or Backspace to refresh", W // 2, H - 90)

bg.convert("RGB").save(os.path.join(OUT, "PREVIEW.png"))
print("PREVIEW.png ok")
