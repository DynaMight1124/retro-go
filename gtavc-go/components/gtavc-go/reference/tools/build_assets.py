#!/usr/bin/env python3
"""Generates every resource of the J2ME GTA Vice City demake into ../res.
Drop your own PNGs over the generated ones (same file names) to re-skin the game."""
import os, sys, math, colorsys
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageChops

HERE = os.path.dirname(os.path.abspath(__file__))
RES = os.path.join(HERE, '..', 'res')
UP = os.environ.get('UPLOADS', '/mnt/user-data/uploads') + '/'
os.makedirs(RES, exist_ok=True)
rng = np.random.RandomState(7)


def save(img, name, pal=None):
    p = os.path.join(RES, name)
    if isinstance(img, np.ndarray):
        img = Image.fromarray(img.astype(np.uint8))
    if pal and img.mode == 'RGB':
        img = img.quantize(pal, method=Image.MEDIANCUT, dither=Image.NONE)
    img.save(p, optimize=True)


def noise(h, w, amp):
    return rng.randint(-amp, amp + 1, (h, w, 1))


def base(h, w, col, amp=6):
    a = np.zeros((h, w, 3), int) + np.array(col)
    a = a + noise(h, w, amp)
    return a


def clip(a):
    return np.clip(a, 0, 255).astype(np.uint8)


# ---------------------------------------------------------------- ground
def tex_road():
    a = base(64, 64, (64, 64, 70), 3)
    for y in range(64):
        a[y, 31] = (234, 192, 40); a[y, 33] = (234, 192, 40)
        a[y, 3] = (205, 205, 205); a[y, 61] = (205, 205, 205)
        if y < 32:
            a[y, 13] = (205, 205, 205); a[y, 51] = (205, 205, 205)
    return clip(a)


def tex_asph():
    return clip(base(64, 64, (70, 70, 76), 3))


def tex_sidewalk():
    a = base(64, 64, (196, 190, 176), 7)
    a[:, 0] = (150, 145, 135); a[0, :] = (150, 145, 135)
    a[:, 32] = (165, 160, 148); a[32, :] = (165, 160, 148)
    return clip(a)


def tex_grass():
    a = base(64, 64, (52, 122, 62), 14)
    return clip(a)


def tex_sand():
    return clip(base(64, 64, (238, 218, 168), 9))


def tex_water():
    a = base(64, 64, (28, 138, 172), 5)
    for i in range(40):
        y = rng.randint(0, 64); x = rng.randint(0, 56); l = rng.randint(4, 14)
        a[y, x:x + l] = (120, 205, 225)
    return clip(a)


def tex_roof():
    return clip(base(64, 64, (120, 118, 116), 12))


# ---------------------------------------------------------------- facades (64x64 = 8m x 4m)
FAC = [(255, 168, 200), (170, 234, 200), (150, 220, 236), (250, 234, 150), (255, 200, 160), (200, 182, 240),
       (240, 240, 234), (255, 150, 140)]
TRIM = [(60, 200, 190), (255, 120, 180), (255, 120, 180), (60, 190, 200), (60, 200, 190), (255, 230, 120),
        (255, 120, 180), (60, 200, 200)]


def facade(col, trim, night):
    a = base(64, 64, col, 4)
    a[0:3, :] = (245, 245, 240)               # cornice
    a[60:64, :] = np.array(trim) * 0.9          # neon-ish trim
    for k, x0 in enumerate((7, 38)):
        wnd = np.zeros((26, 19, 3), int)
        if night and (k == 0 or (col[0] + col[1]) % 2 == 0):
            wnd[:] = (255, 214, 120)
            wnd[:, ::6] = (230, 170, 80)
        else:
            wnd[:] = (36, 78, 100)
            for i in range(26):
                wnd[i, max(0, 14 - i // 2):max(0, 14 - i // 2) + 3] = (90, 150, 170)
        a[14:40, x0:x0 + 19] = wnd
        a[12:14, x0 - 1:x0 + 20] = (250, 250, 245)
        a[40:42, x0 - 2:x0 + 21] = (250, 250, 245)
    a[:, 0:2] = (250, 250, 245)
    return clip(a)


def shop(col, night, stripe):
    a = base(64, 64, col, 4)
    a[0:12, :] = (250, 250, 245)
    for x in range(0, 64, 8):
        a[0:12, x:x + 4] = stripe
    a[12:15, :] = (40, 40, 44)
    glass = np.zeros((40, 56, 3), int)
    glass[:] = (255, 205, 130) if night else (44, 96, 118)
    for i in range(40):
        glass[i, max(0, 30 - i):max(0, 30 - i) + 4] = (150, 200, 215) if not night else (255, 240, 190)
    a[17:57, 4:60] = glass
    a[17:19, 4:60] = (250, 250, 245)
    a[56:58, 4:60] = (250, 250, 245)
    for x in (4, 31, 58):
        a[17:58, x:x + 2] = (250, 250, 245)
    a[58:64, :] = (190, 186, 175)
    return clip(a)


def tex_hotel(night):
    a = base(64, 64, (255, 126, 176), 4)
    a[0:3, :] = (250, 250, 250)
    a[60:64, :] = (255, 255, 255)
    for x0 in (4, 36):
        a[10:44, x0:x0 + 24] = (255, 255, 255)
        w = np.zeros((30, 20, 3), int)
        w[:] = (255, 210, 120) if night else (48, 70, 96)
        for i in range(30):
            w[i, max(0, 16 - i // 2):max(0, 16 - i // 2) + 3] = (255, 240, 180) if night else (110, 150, 180)
        a[12:42, x0 + 2:x0 + 22] = w
    a[:, 30:34] = (255, 255, 255)
    return clip(a)


def tex_sign():
    img = Image.new('RGB', (128, 32), (10, 6, 14))
    d = ImageDraw.Draw(img)
    f = ImageFont.truetype('/usr/share/fonts/opentype/noto/NotoSansCJK-Black.ttc', 22)
    glow = Image.new('RGB', (128, 32), (0, 0, 0))
    gd = ImageDraw.Draw(glow)
    t = 'OCEAN VIEW'
    w = d.textlength(t, font=f)
    gd.text(((128 - w) / 2, 2), t, font=f, fill=(255, 40, 160))
    glow = glow.filter(ImageFilter.GaussianBlur(1.6))
    img = ImageChops.add(img, glow)
    d = ImageDraw.Draw(img)
    d.text(((128 - w) / 2, 2), t, font=f, fill=(255, 220, 240))
    return img


# ---------------------------------------------------------------- lobby
def tex_marble():
    a = base(64, 64, (22, 22, 28), 6)
    for i in range(22):
        a[rng.randint(0, 64), rng.randint(0, 64)] = (70, 70, 80)
    for i in range(3):
        x = rng.randint(0, 64)
        for y in range(64):
            x = (x + rng.randint(-1, 2)) % 64
            a[y, x] = (70, 70, 80)
    return clip(a)


def tex_border():
    a = np.zeros((64, 64, 3), int)
    for y in range(64):
        for x in range(64):
            c = ((x // 8) + (y // 8)) % 2
            a[y, x] = (200, 215, 190) if c else (60, 140, 110)
    return clip(a + noise(64, 64, 4))


def tex_lwall():
    return clip(base(64, 64, (48, 48, 54), 10))


def tex_paint():
    a = np.zeros((64, 64, 3), int) + (16, 16, 18)
    blocks = [((0, 0, 28, 14), (236, 188, 40)), ((0, 14, 28, 36), (20, 20, 22)), ((28, 0, 64, 20), (64, 140, 196)),
              ((28, 20, 64, 40), (90, 160, 120)), ((0, 36, 28, 64), (236, 188, 40)), ((28, 40, 64, 52), (230, 230, 220)),
              ((28, 52, 64, 64), (60, 130, 200)), ((50, 0, 64, 8), (200, 60, 50))]
    for (x0, y0, x1, y1), c in blocks:
        a[y0:y1, x0:x1] = c
    return clip(a)


def tex_door():
    a = base(64, 64, (212, 206, 172), 4)
    for x0 in (4, 34):
        a[6:58, x0:x0 + 26] = (150, 196, 196)
        a[6:58, x0:x0 + 2] = (230, 226, 200)
    return clip(a)


def tex_lh():
    a = np.zeros((64, 64, 3), int)
    for y in range(64):
        a[y, :] = (214, 40, 44) if (y // 16) % 2 == 0 else (240, 240, 236)
    return clip(a + noise(64, 64, 4))


def tex_trunk():
    a = base(64, 32, (112, 82, 52), 8)
    for y in range(0, 64, 5):
        a[y, :] = (80, 58, 38)
    return clip(a)


def tex_frond():
    S = 64
    im = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    cy = S // 2
    d.line([(0, cy), (S - 2, cy)], fill=(70, 120, 50, 255), width=2)
    for x in range(3, S - 4, 3):
        L = int((S / 2 - 3) * (1 - abs(x - S * 0.35) / (S * 0.75)))
        L = max(L, 3)
        shade = (28 + (x % 9) * 3, 104 + (x % 7) * 5, 40, 255)
        d.line([(x, cy), (x + 5, cy - L)], fill=shade, width=2)
        d.line([(x, cy), (x + 5, cy + L)], fill=shade, width=2)
    a = np.array(im)
    # make keyed pixels exactly alpha 0 / 255
    a[..., 3] = np.where(a[..., 3] > 100, 255, 0)
    return Image.fromarray(a, 'RGBA')


# ---------------------------------------------------------------- pedestrians (recolors of Tommy's texture)
def ped_variants():
    src = Image.open(UP + 'player.png').convert('RGBA')
    full = np.array(src.resize((256, 256), Image.LANCZOS)).astype(float)
    outs = {}

    def variant(name, size, shirt_h=None, shirt_s=1.0, shirt_v=1.0, pants=None, skin=1.0, hair=None, white_shirt=False):
        a = full.copy()
        rgb = a[..., :3] / 255.0
        h = np.zeros(rgb.shape[:2]); s = np.zeros_like(h); v = np.zeros_like(h)
        for y in range(256):
            for x in range(256):
                hh, ss, vv = colorsys.rgb_to_hsv(*rgb[y, x]); h[y, x] = hh; s[y, x] = ss; v[y, x] = vv
        yy, xx = np.mgrid[0:256, 0:256]
        shirt = (xx >= 86) & (xx < 190) & (yy >= 122) & (s > 0.12) & (h > 0.35) & (h < 0.65)
        jeans = (xx < 90) & (yy >= 84) & (yy < 256)
        skin_m = (xx >= 55) & (yy < 150) & (s > 0.2) & (h < 0.12) & (v > 0.35)
        h2, s2, v2 = h.copy(), s.copy(), v.copy()
        if white_shirt:
            s2[shirt] = 0.04; v2[shirt] = np.clip(v[shirt] * 1.9, 0, 1)
        elif shirt_h is not None:
            h2[shirt] = shirt_h; s2[shirt] = np.clip(s[shirt] * shirt_s, 0, 1); v2[shirt] = np.clip(v[shirt] * shirt_v, 0, 1)
        if pants is not None:
            ph, ps, pv = pants
            h2[jeans] = ph; s2[jeans] = ps; v2[jeans] = np.clip(v[jeans] * pv * 1.3, 0, 1)
        v2[skin_m] = np.clip(v[skin_m] * skin, 0, 1)
        if hair is not None:
            hm = (xx >= 125) & (yy < 70) & (v < 0.3)
            h2[hm] = hair[0]; s2[hm] = hair[1]; v2[hm] = np.maximum(v[hm], hair[2])
        res = a.copy()
        for y in range(256):
            for x in range(256):
                if (shirt[y, x] and (shirt_h is not None or white_shirt)) or (pants is not None and jeans[y, x]) \
                        or skin_m[y, x] or (hair is not None and (x >= 125 and y < 70 and v[y, x] < 0.3)):
                    res[y, x, :3] = np.array(colorsys.hsv_to_rgb(h2[y, x] % 1.0, s2[y, x], v2[y, x])) * 255
        img = Image.fromarray(res.astype(np.uint8), 'RGBA').resize((size, size), Image.LANCZOS).convert('RGB')
        outs[name] = img

    outs['ped0'] = Image.fromarray(full.astype(np.uint8), 'RGBA').resize((128, 128), Image.LANCZOS).convert('RGB')
    variant('ped1', 64, shirt_h=0.0, shirt_s=1.2, shirt_v=1.15, pants=(0.12, 0.35, 0.85))
    variant('ped2', 64, shirt_h=0.14, shirt_s=1.3, shirt_v=1.35, pants=(0.0, 0.0, 1.6), skin=1.1)
    variant('ped3', 64, shirt_h=0.9, shirt_s=1.0, shirt_v=1.2, pants=(0.0, 0.0, 0.25), skin=0.62, hair=(0.0, 0.0, 0.08))
    variant('ped4', 64, white_shirt=True, skin=1.18, hair=(0.12, 0.6, 0.62))
    variant('ped5', 64, shirt_h=0.62, shirt_s=1.3, shirt_v=0.55, pants=(0.62, 0.55, 0.55), skin=1.05)
    return outs


# ---------------------------------------------------------------- UI
def build_font():
    fpath = '/usr/share/fonts/opentype/noto/NotoSansCJK-Black.ttc'
    S = 4
    CW, CH, BASE = 14, 16, 12
    big = ImageFont.truetype(fpath, 13 * S)
    chars = [chr(c) for c in range(32, 127)]
    atlas = Image.new('RGBA', (CW * len(chars), CH), (0, 0, 0, 0))
    for i, ch in enumerate(chars):
        if ch == ' ':
            continue
        g = Image.new('L', (CW * S, CH * S), 0)
        d = ImageDraw.Draw(g)
        bb = d.textbbox((0, BASE * S), ch, font=big, anchor='ls')
        d.text((-bb[0], BASE * S), ch, font=big, fill=255, anchor='ls')
        small = g.resize((CW, CH), Image.LANCZOS)
        a = np.array(small).astype(float)
        a = np.where(a > 40, np.clip(a * 1.4, 0, 255), 0).astype(np.uint8)
        rgba = np.zeros((CH, CW, 4), np.uint8)
        rgba[..., :3] = 255
        rgba[..., 3] = a
        atlas.paste(Image.fromarray(rgba, 'RGBA'), (i * CW, 0))
    atlas.save(os.path.join(RES, 'font.png'), optimize=True)


def sprite_heart(S=14):
    k = 8
    im = Image.new('RGBA', (S * k, S * k), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    r = S * k * 0.27
    d.ellipse([S * k * 0.06, S * k * 0.1, S * k * 0.06 + 2 * r, S * k * 0.1 + 2 * r], fill=(250, 150, 226, 255))
    d.ellipse([S * k * 0.94 - 2 * r, S * k * 0.1, S * k * 0.94, S * k * 0.1 + 2 * r], fill=(250, 150, 226, 255))
    d.polygon([(S * k * 0.07, S * k * 0.42), (S * k * 0.93, S * k * 0.42), (S * k * 0.5, S * k * 0.95)], fill=(250, 150, 226, 255))
    return im.resize((S, S), Image.LANCZOS)


def sprite_star(col, S=18):
    k = 8
    im = Image.new('RGBA', (S * k, S * k), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    c = S * k / 2
    for i in range(6):
        a = math.radians(i * 60 - 90)
        x, y = c + math.cos(a) * S * k * 0.30, c + math.sin(a) * S * k * 0.30
        r = S * k * 0.17
        d.ellipse([x - r, y - r, x + r, y + r], fill=col)
    d.regular_polygon((c, c, S * k * 0.28), 6, rotation=0, fill=col)
    return im.resize((S, S), Image.LANCZOS)


def sprite_weapon(W=38, H=30):
    k = 6
    im = Image.new('RGBA', (W * k, H * k), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([0, 0, W * k - 1, H * k - 1], radius=5 * k, outline=(248, 150, 226, 255), width=3 * k)
    d.rounded_rectangle([3 * k, 3 * k, W * k - 3 * k, H * k - 3 * k], radius=3 * k, outline=(255, 215, 245, 255), width=1 * k)
    # fist: gradient cyan -> magenta
    fx, fy, fw, fh = 9 * k, 4 * k, 20 * k, 22 * k
    fist = Image.new('L', im.size, 0)
    fd = ImageDraw.Draw(fist)
    for i in range(4):   # knuckles
        x0 = fx + i * 4.4 * k
        fd.rounded_rectangle([x0, fy + (1 if i in (1, 2) else 2.5) * k, x0 + 5 * k, fy + 10 * k], radius=2 * k, fill=255)
    fd.rounded_rectangle([fx + 1 * k, fy + 6 * k, fx + 19 * k, fy + 15 * k], radius=3 * k, fill=255)
    fd.polygon([(fx + 3 * k, fy + 12 * k), (fx + 19 * k, fy + 12 * k), (fx + 22 * k, fy + 22 * k), (fx + 10 * k, fy + 22 * k)], fill=255)
    fd.ellipse([fx - 3 * k, fy + 8 * k, fx + 6 * k, fy + 16 * k], fill=255)   # thumb
    grad = Image.new('RGBA', im.size)
    gp = grad.load()
    for y in range(im.size[1]):
        t = min(1, max(0, (y - fy) / fh))
        c = (int(90 + 160 * t), int(235 - 120 * t), int(240 - 10 * t), 255)
        for x in range(im.size[0]):
            gp[x, y] = c
    im.paste(grad, (0, 0), fist)
    return im.resize((W, H), Image.LANCZOS)


def build_hud():
    sheet = Image.new('RGBA', (128, 64), (0, 0, 0, 0))
    sheet.paste(sprite_heart(14), (0, 0))
    sheet.paste(sprite_star((24, 88, 128, 255), 18), (16, 0))
    sheet.paste(sprite_star((120, 200, 240, 255), 18), (36, 0))
    sheet.paste(sprite_weapon(38, 30), (0, 20))
    sheet.save(os.path.join(RES, 'hud.png'), optimize=True)


def build_logo():
    im = Image.open(UP + '1000364947.jpg').convert('RGB').crop((70, 10, 480, 195))
    a = np.array(im).astype(int)
    mx = a.max(axis=2); mn = a.min(axis=2)
    mask = (mx > 188) | ((a[..., 0] > 200) & (a[..., 1] < 190) & (a[..., 2] > 150) & (mx - mn > 40))
    m = Image.fromarray((mask * 255).astype(np.uint8))
    dil = m.filter(ImageFilter.MaxFilter(5))
    out = np.zeros(a.shape[:2] + (4,), np.uint8)
    out[..., 3] = np.array(dil)
    out[..., :3] = 0
    mm = np.array(m) > 0
    out[mm, :3] = a[mm]
    img = Image.fromarray(out, 'RGBA')
    bb = img.getbbox()
    img = img.crop(bb)
    w = 100
    img = img.resize((w, int(img.size[1] * w / img.size[0])), Image.LANCZOS)
    arr = np.array(img)
    arr[..., 3] = np.where(arr[..., 3] > 110, 255, 0)
    Image.fromarray(arr, 'RGBA').save(os.path.join(RES, 'logo.png'), optimize=True)


def build_menu_bg():
    W, H = 320, 240
    n = rng.rand(48, 64)
    big = Image.fromarray((n * 255).astype(np.uint8)).resize((W, H), Image.BICUBIC).filter(ImageFilter.GaussianBlur(1.2))
    g = np.array(big)
    camo = np.where(g > 128, 2, 1).astype(np.uint8)       # 1 dark, 2 light
    pal = Image.new('P', (W, H))
    mask = Image.new('L', (W, H), 0)
    ImageDraw.Draw(mask).polygon([(40, 19), (301, 36), (307, 188), (32, 214)], fill=255)
    m = np.array(mask) > 0
    idx = np.where(m, camo, 0).astype(np.uint8)
    pal = Image.fromarray(idx, 'P')
    pal.putpalette([0, 0, 0, 88, 84, 96, 122, 120, 132] + [0] * (256 * 3 - 9))
    pal.save(os.path.join(RES, 'menu_bg.png'), optimize=True)


def crop43(im, box):
    return im.crop(box).resize((320, 240), Image.LANCZOS)


def build_screens():
    a = Image.open(UP + '1000364945.jpg').convert('RGB')
    save(crop43(a, (250, 0, 1850, 1200)), 'ls0.png', 96)
    b = Image.open(UP + '1000364946.jpg').convert('RGB')
    save(crop43(b, (250, 0, 1690, 1080)), 'ls1.png', 96)
    c = Image.open(UP + '1000364944.webp').convert('RGB')
    save(crop43(c, (200, 0, 3400, 2400)), 'ls2.png', 96)
    d = Image.open(UP + '1000364943.jpg').convert('RGB')
    save(d.resize((320, 240), Image.LANCZOS), 'ls3.png', 96)
    e = Image.open(UP + '1000364942.jpg').convert('RGB')
    save(e.resize((320, 240), Image.LANCZOS), 'intro.png', 96)


def build_icon():
    im = Image.new('RGB', (32, 32), (255, 110, 190))
    d = ImageDraw.Draw(im)
    for y in range(32):
        d.line([(0, y), (32, y)], fill=(255 - y * 2, 100 + y * 4, 200 + y))
    f = ImageFont.truetype('/usr/share/fonts/opentype/noto/NotoSansCJK-Black.ttc', 17)
    d.text((3, 5), 'VC', font=f, fill=(255, 255, 255))
    im.save(os.path.join(RES, 'icon.png'))


def main():
    T = {'road': tex_road(), 'asph': tex_asph(), 'sidewalk': tex_sidewalk(), 'grass': tex_grass(),
         'sand': tex_sand(), 'water': tex_water(), 'roof': tex_roof(), 'marble': tex_marble(),
         'border': tex_border(), 'lwall': tex_lwall(), 'paint': tex_paint(), 'door': tex_door(),
         'lh': tex_lh(), 'trunk': tex_trunk()}
    for i in range(8):
        T['fac%d' % i] = facade(FAC[i], TRIM[i], False)
        T['nfac%d' % i] = facade(FAC[i], TRIM[i], True)
    T['shop0'] = shop((250, 215, 150), False, (60, 170, 90)); T['nshop0'] = shop((250, 215, 150), True, (60, 170, 90))
    T['shop1'] = shop((170, 230, 235), False, (255, 120, 180)); T['nshop1'] = shop((170, 230, 235), True, (255, 120, 180))
    T['hotel'] = tex_hotel(False); T['nhotel'] = tex_hotel(True)
    for k, v in T.items():
        save(v, 't_%s.png' % k, 48 if k not in ('road',) else 24)
    tex_sign().save(os.path.join(RES, 't_sign.png'), optimize=True)
    tex_frond().save(os.path.join(RES, 't_frond.png'), optimize=True)
    for k, v in ped_variants().items():
        save(v, 't_%s.png' % k, 128 if k == 'ped0' else 64)
    build_font(); build_hud(); build_logo(); build_menu_bg(); build_screens(); build_icon()
    # Tommy models: LOD0 (player) and LOD1 (pedestrians)
    sys.path.insert(0, HERE)
    import mesh_tools
    obj = UP + 'Tommy_Vercetti.obj'
    mesh_tools.export(obj, os.path.join(RES, 'ped0.bin'), 0.34)
    mesh_tools.export(obj, os.path.join(RES, 'ped1.bin'), 0.16)
    print('assets done:', len(os.listdir(RES)), 'files')


if __name__ == '__main__':
    main()
