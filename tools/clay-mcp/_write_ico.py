#!/usr/bin/env python3
"""Pack PNG files into Windows ICO (PNG-compressed images)."""
from __future__ import annotations

import struct
from pathlib import Path

OUT = Path(r"T:\Apps\clay\tools\clay-mcp")


def write_ico(dest: Path, png_paths: list[Path]) -> None:
    entries = []
    blobs = []
    for path in png_paths:
        data = path.read_bytes()
        if data[:8] != b"\x89PNG\r\n\x1a\n":
            raise RuntimeError(f"not a PNG: {path}")
        # IHDR width/height
        w = struct.unpack(">I", data[16:20])[0]
        h = struct.unpack(">I", data[20:24])[0]
        entries.append((w, h, len(data)))
        blobs.append(data)

    count = len(entries)
    offset = 6 + 16 * count
    buf = bytearray()
    buf += struct.pack("<HHH", 0, 1, count)
    for (w, h, size), blob in zip(entries, blobs):
        buf += struct.pack(
            "<BBBBHHII",
            w if w < 256 else 0,
            h if h < 256 else 0,
            0,
            0,
            1,
            32,
            size,
            offset,
        )
        offset += size
    for blob in blobs:
        buf += blob
    dest.write_bytes(buf)


def main() -> None:
    p = lambda n: OUT / f"clay-logo({n}).png"
    write_ico(
        OUT / "clay-logo.ico",
        [p(256), p(128), p(64), p(32), p(16)],
    )
    write_ico(OUT / "clay-logo(16).ico", [p(16)])
    fav = OUT / "favicon.png"
    fav.write_bytes(p(32).read_bytes())
    write_ico(OUT / "favicon.ico", [p(32), p(16)])
    for name in ("clay-logo.ico", "clay-logo(16).ico", "favicon.png", "favicon.ico"):
        f = OUT / name
        print(f"{name:22} {f.stat().st_size:8d}")


if __name__ == "__main__":
    main()
