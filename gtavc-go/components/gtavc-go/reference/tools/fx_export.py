#!/usr/bin/env python3
"""Particles / sky sprites, blood decal and bitmap fonts -> res/.

  fx_*.png    additive / intensity sprites (grey = intensity), from assets/particles (particle.txd export)
  t_blood.png blood pool decal (from assets/particles/blood_user.png, white background keyed out)
  font.png    menu + HUD font   <- assets/fonts/hud.ttf (or hud.otf) when present, otherwise the
              existing res/font.png is kept
  font_tip.png help/tip font (regular weight) <- assets/fonts/NotoSans-Regular.ttf
Atlas layout used by Ui.java: 95 glyphs (ASCII 32..126), 14x16 cells, alpha = coverage.
"""
import os, glob
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
RES = os.path.join(ROOT, 'res')
PART = os.path.join(ROOT, 'assets', 'particles')
FONTS = os.path.join(ROOT, 'assets', 'fonts')

SPRITES = {  # name: (source, size)
    'cloud1': ('cloud1', 64), 'cloud2': ('cloud2', 64), 'cloud3': ('cloud3', 64), 'cloudhilit': ('cloudhilit', 64),
    'corona': ('corona', 64), 'star': ('coronastar', 32), 'moon': ('coronamoon', 32), 'ringa': ('coronaringa', 64),
    'hex': ('coronahex', 32), 'flash': ('gunflash1', 32), 'smoke': ('smoke1', 32), 'fire': ('flame1', 32), 'explo': ('explo02', 64), 'target': ('target256', 64), 'circle': ('coronacircle', 32), 'reflect': ('coronareflect', 32), 'streek': ('streek', 32),
}


def sprites():
    for n, (src, s) in SPRITES.items():
        im = Image.open(os.path.join(PART, src + '.png')).convert('RGB').resize((s, s), Image.LANCZOS)
        g = np.asarray(im).max(2)
        Image.fromarray(g.astype(np.uint8), 'L').save(os.path.join(RES, 'fx_%s.png' % n), optimize=True)


def blood():
    a = np.asarray(Image.open(os.path.join(PART, 'blood_user.png')).convert('RGB')).astype(float)
    whiteness = a.min(2)                     # white background -> 255
    alpha = np.clip((235 - whiteness) * 2.2, 0, 255)
    rgb = np.clip(a * 0.85, 0, 255)
    out = np.dstack([rgb, alpha]).astype(np.uint8)
    im = Image.fromarray(out, 'RGBA').resize((32, 32), Image.LANCZOS)
    o = np.array(im); o[..., 3] = np.where(o[..., 3] > 100, 255, 0)
    Image.fromarray(o, 'RGBA').save(os.path.join(RES, 't_blood.png'), optimize=True)


def font(path, out, size=13, boost=1.4, cut=40):
    S, CW, CH, BASE = 4, 14, 16, 12
    big = ImageFont.truetype(path, size * S)
    atlas = Image.new('RGBA', (CW * 95, CH), (0, 0, 0, 0))
    for i, ch in enumerate(chr(c) for c in range(32, 127)):
        if ch == ' ': continue
        g = Image.new('L', (CW * S, CH * S), 0)
        d = ImageDraw.Draw(g)
        bb = d.textbbox((0, BASE * S), ch, font=big, anchor='ls')
        d.text((-bb[0], BASE * S), ch, font=big, fill=255, anchor='ls')
        a = np.array(g.resize((CW, CH), Image.LANCZOS)).astype(float)
        a = np.where(a > cut, np.clip(a * boost, 0, 255), 0).astype(np.uint8)
        rgba = np.zeros((CH, CW, 4), np.uint8); rgba[..., :3] = 255; rgba[..., 3] = a
        atlas.paste(Image.fromarray(rgba, 'RGBA'), (i * CW, 0))
    atlas.save(os.path.join(RES, out), optimize=True)


if __name__ == '__main__':
    os.makedirs(RES, exist_ok=True)
    sprites(); blood()
    hud = [p for p in glob.glob(os.path.join(FONTS, 'hud.*')) if p.lower().endswith(('.ttf', '.otf'))]
    if hud:
        font(hud[0], 'font.png', 16, 1.25, 12); print('menu/HUD font from', os.path.basename(hud[0]))
    else:
        print('no assets/fonts/hud.ttf - keeping res/font.png')
    font(os.path.join(FONTS, 'NotoSans-Regular.ttf'), 'font_tip.png', 12, 1.15)
    print('fx sprites, blood decal, tip font done')
