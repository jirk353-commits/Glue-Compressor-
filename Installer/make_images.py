"""Draws Drum Glue installer art: steel faceplate, red-capped knob, engraved title."""
import math, sys, random
from PIL import Image, ImageDraw, ImageFont, ImageFilter
fontpath = sys.argv[1]

def plate(w, h):
    img = Image.new('RGBA', (w, h)); d = ImageDraw.Draw(img)
    for y in range(h):
        t = y / h
        d.line([(0, y), (w, y)], fill=(int(74 - 30*t), int(88 - 35*t), int(104 - 40*t), 255))
    rnd = random.Random(5)
    grain = Image.new('RGBA', (w, h), (0, 0, 0, 0)); gd = ImageDraw.Draw(grain)
    for y in range(h):
        for _ in range(3):
            x0 = rnd.random() * w; ln = 20 + rnd.random() * 120
            c = 255 if rnd.random() < 0.5 else 0
            gd.line([(x0, y), (x0 + ln, y)], fill=(c, c, c, int(8 + rnd.random() * 14)))
    img.alpha_composite(grain)
    return img

def knob(img, cx, cy, r):
    d = ImageDraw.Draw(img)
    sh = Image.new('RGBA', img.size, (0, 0, 0, 0)); ImageDraw.Draw(sh).ellipse([cx-r, cy-r+r*0.12, cx+r, cy+r+r*0.12], fill=(0, 0, 0, 170))
    img.alpha_composite(sh.filter(ImageFilter.GaussianBlur(r * 0.12)))
    d = ImageDraw.Draw(img)
    for i in range(11):
        a = math.radians(135 + i * 27)
        r1, r2 = r * 1.18, r * (1.32 if i % 5 == 0 else 1.26)
        d.line([cx + math.cos(a) * r1, cy + math.sin(a) * r1, cx + math.cos(a) * r2, cy + math.sin(a) * r2], fill=(240, 236, 226), width=max(2, int(r * 0.05)))
    d.ellipse([cx-r, cy-r, cx+r, cy+r], fill=(160, 166, 174), outline=(30, 32, 36), width=2)
    br = r * 0.78
    d.ellipse([cx-br, cy-br, cx+br, cy+br], fill=(18, 19, 22))
    cr = br * 0.45
    d.ellipse([cx-cr, cy-cr, cx+cr, cy+cr], fill=(255, 90, 71))
    a = math.radians(300)
    d.line([cx + math.cos(a) * cr, cy + math.sin(a) * cr, cx + math.cos(a) * r * 0.95, cy + math.sin(a) * r * 0.95], fill=(255, 255, 255), width=max(3, int(r * 0.08)))

def engraved(d, xy, s, f, col):
    x, y = xy
    d.text((x, y - 1), s, font=f, fill=(0, 0, 0, 200))
    d.text((x, y + 1), s, font=f, fill=(255, 255, 255, 60))
    d.text((x, y), s, font=f, fill=col)

W, H = 328, 628
img = plate(W, H); knob(img, W // 2, 220, 95)
d = ImageDraw.Draw(img)
f1 = ImageFont.truetype(fontpath, 40); f2 = ImageFont.truetype(fontpath, 22); f3 = ImageFont.truetype(fontpath, 14)
def c(y, s, f, col): engraved(d, ((W - d.textlength(s, font=f)) / 2, y), s, f, col)
c(400, "DRUM GLUE", f1, (255, 244, 220))
c(458, "ANALOG BUS COMPRESSOR", f3, (198, 204, 212))
d.line([(60, 525), (W - 60, 525)], fill=(20, 24, 28), width=2)
c(550, "ROHIT JI", f2, (255, 181, 46))
img.convert('RGB').save('wizard-large.bmp'); img.convert('RGB').resize((164, 314), Image.LANCZOS).save('wizard-large-100.bmp')
s = plate(110, 110); knob(s, 55, 55, 34)
s.convert('RGB').save('wizard-small.bmp'); s.convert('RGB').resize((55, 55), Image.LANCZOS).save('wizard-small-100.bmp')
ic = Image.new('RGBA', (512, 512), (0, 0, 0, 0))
m = Image.new('L', (480, 480), 0); ImageDraw.Draw(m).rounded_rectangle([0, 0, 479, 479], 100, fill=255)
ic.paste(plate(480, 480), (16, 16), m); knob(ic, 256, 256, 150)
ic.save('icon.ico', sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
