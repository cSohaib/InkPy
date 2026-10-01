"""Host-only contact sheet; pip install Pillow. PBMs are the actual one-bit output."""
import csv
import sys
from pathlib import Path
from PIL import Image, ImageDraw

report, images, output = sys.argv[1:]
rows = list(csv.DictReader(open(report), delimiter="\t"))
heights = [max(72, int(r["height"]) + 50) for r in rows]
canvas = Image.new("RGB", (960, sum(heights[::2]) + sum(heights[1::2]) + 40), "white")
draw = ImageDraw.Draw(canvas)
y = [10, 10]
for i, row in enumerate(rows):
    col = i % 2
    x = col * 480
    top = y[col]
    draw.text((x + 8, top), f'{row["id"]}. {row["style"]} / {row["status"]}', fill="black")
    if row["status"] == "rendered":
        image = Image.open(Path(images) / f'{row["id"]}.pbm').convert("RGB")
        canvas.paste(image, (x, top + 18))
        height = image.height + 40
    else:
        draw.text((x + 8, top + 20), row["error"], fill="black")
        source = (Path(images) / f'{row["id"]}.tex').read_text().strip()
        draw.text((x + 8, top + 36), source[:65], fill="black")
        height = 72
    y[col] += height
    draw.line((x + 8, y[col] - 5, x + 472, y[col] - 5), fill="#bbbbbb")
canvas.crop((0, 0, 960, max(y) + 5)).save(output)
