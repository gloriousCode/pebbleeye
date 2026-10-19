#!/usr/bin/env python3
"""Draw original PebbleEye branding at final store sizes. No image resizing."""
import hashlib
import json
import math
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
STORE = ROOT / "store"
FONT = ROOT / "resources/fonts/Roboto-Bold.ttf"


def dial(draw, cx, cy, radius, digital=True):
    """Draw the recognisable segmented bezel and hands as native primitives."""
    warm = ["#ff3333", "#ff6600", "#ff9900", "#ffbb33", "#ffdd55", "#ffff99"]
    cool = ["#0033aa", "#0055cc", "#0088ff", "#33aaff", "#55bbff", "#99ddff"]
    width = max(4, round(radius * .16))
    for starts, colours in [([309, 288, 267, 246, 225, 207], warm), ([39, 60, 81, 102, 123, 141], cool)]:
        for angle, colour in zip(starts, colours):
            points = []
            for r, angles in [(radius, range(angle, angle + 13)), (radius - width, range(angle + 12, angle - 1, -1))]:
                points += [(round(cx + r * math.sin(math.radians(a))), round(cy - r * math.cos(math.radians(a)))) for a in angles]
            draw.polygon(points, fill=colour)
    inner = round(radius * .65)
    draw.ellipse((cx-inner, cy-inner, cx+inner, cy+inner), fill="#073d25", outline="#2c8451", width=1)
    for angle in range(0, 360, 30):
        points = [(round(cx + r * math.sin(math.radians(angle))), round(cy - r * math.cos(math.radians(angle)))) for r in (inner - max(3, radius*.055), inner - 2)]
        draw.line(points, fill="#aaffaa", width=max(1, round(radius*.015)))
    for angle, length, thickness in [(304, inner*.57, max(2,round(radius*.045))), (48, inner*.81, max(2,round(radius*.03)))]:
        end = (round(cx + length*math.sin(math.radians(angle))), round(cy-length*math.cos(math.radians(angle))))
        draw.line(((cx,cy),end), fill="#effff2", width=thickness)
    hub=max(2,round(radius*.035)); draw.ellipse((cx-hub,cy-hub,cx+hub,cy+hub),fill="#ff6655")
    if digital:
        font=ImageFont.truetype(str(FONT), max(10,round(radius*.23)))
        text="10:08"; box=draw.textbbox((0,0),text,font=font); tw=box[2]-box[0]; th=box[3]-box[1]
        x=cx-tw/2; y=cy+radius*.22
        draw.rounded_rectangle((x-4,y-3,x+tw+4,y+th+3),radius=2,fill="#06100b")
        draw.text((x,y-box[1]),text,font=font,fill="#aaffaa")


def main():
    assets=[]
    for size in (80,144):
        image=Image.new("RGB",(size,size),"#080e0b")
        draw=ImageDraw.Draw(image)
        dial(draw,size//2,size//2,round(size*.45))
        path=STORE/f"pebbleeye-icon-{size}.png"; image.save(path)
        assets.append(path)
    image=Image.new("RGB",(720,320),"#080e0b"); draw=ImageDraw.Draw(image)
    # Typography and the dial are drawn directly into the native banner canvas.
    draw.line((40,32,680,32),fill="#254433",width=1)
    draw.text((40,53),"PEBBLEEYE",font=ImageFont.truetype(str(FONT),48),fill="#f1fff5")
    draw.text((42,130),"Your time.",font=ImageFont.truetype(str(FONT),32),fill="#aaffaa")
    draw.text((42,171),"Your mission.",font=ImageFont.truetype(str(FONT),32),fill="#aaffaa")
    draw.text((42,267),"TIME 2  /  ROUND 2",font=ImageFont.truetype(str(FONT),15),fill="#a0b5a8")
    dial(draw,565,160,124)
    path=STORE/"pebbleeye-banner.png"; image.save(path); assets.append(path)
    manifest={"artwork":"Original native-size promotional graphics; not watch screenshots", "assets":[]}
    for path in assets:
        with Image.open(path) as check:
            manifest["assets"].append({"file":path.name,"width":check.width,"height":check.height,
                "sha256":hashlib.sha256(path.read_bytes()).hexdigest()})
    (STORE/"artwork-manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    print("Native 720×320 banner and 80×80 / 144×144 icons verified")


if __name__ == "__main__":
    main()
