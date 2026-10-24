"""Fix white box on exported PNG (fallback when buffer path fails)."""
from pathlib import Path

from PIL import Image

RECT = (472, 376, 472 + 132, 376 + 162)  # left, top, right, bottom
SRC = Path(r"C:\Users\Kurt Steven\AppData\Local\Temp\clay-snap-954at7_8.png")
OUT = Path(r"T:\Apps\clay\tools\clay-mcp\g7_2_landscape_fixed.png")


def main():
    im = Image.open(SRC).convert("RGB")
    px = im.load()
    l, t, r, b = RECT
    for y in range(t, b):
        for x in range(l, r):
            sx = max(0, l - 1 - ((x - l) % 4))
            px[x, y] = px[sx, y]
    im.save(OUT)
    print("saved", OUT)


if __name__ == "__main__":
    main()
