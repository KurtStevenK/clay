#!/usr/bin/env python3
"""Scale clay-logo in Clay (LoHalo, keep DPI) and build Windows/favicon ICO."""
from __future__ import annotations

import json
import os
import socket
import sys
from pathlib import Path

HOST, PORT = "127.0.0.1", 9877
OUT = Path(r"T:\Apps\clay\tools\clay-mcp")
SRC = OUT / "clay-logo.jpg"

CLAY_CODE = r'''
import os
from gi.repository import Gio

outdir = r"T:\Apps\clay\tools\clay-mcp"
src = os.path.join(outdir, "clay-logo.jpg")

images = Clay.get_images() or []
image = None
for im in images:
    name = im.get_name() if hasattr(im, "get_name") else ""
    if "clay-logo" in str(name) and "512" not in str(name):
        image = im
        break
if image is None:
    image = Clay.file_load(Clay.RunMode.NONINTERACTIVE, Gio.File.new_for_path(src))
    Clay.Display.new(image)

prev_interp = Clay.context_get_interpolation()
best = Clay.InterpolationType.LOHALO
Clay.context_set_interpolation(best)

xres = yres = None
if hasattr(image, "get_resolution"):
    try:
        res = image.get_resolution()
        if isinstance(res, tuple) and len(res) >= 2:
            xres, yres = float(res[0]), float(res[1])
    except Exception:
        pass
if xres is None:
    try:
        xres = float(image.get_xresolution())
        yres = float(image.get_yresolution())
    except Exception:
        xres = yres = 72.0

def _set_if(cfg, key, value):
    try:
        cfg.set_property(key, value)
        return True
    except Exception:
        return False

def export_jpeg(image, path):
    pdb = Clay.get_pdb()
    proc = pdb.lookup_procedure("file-jpeg-export")
    if proc is None:
        Clay.file_save(Clay.RunMode.NONINTERACTIVE, image, Gio.File.new_for_path(path), None)
        return "file_save_fallback"
    cfg = proc.create_config()
    _set_if(cfg, "run-mode", Clay.RunMode.NONINTERACTIVE)
    _set_if(cfg, "image", image)
    _set_if(cfg, "file", Gio.File.new_for_path(path))
    _set_if(cfg, "quality", 1.0)
    _set_if(cfg, "smoothing", 0.0)
    _set_if(cfg, "optimize", True)
    _set_if(cfg, "progressive", True)
    _set_if(cfg, "cmyk", False)
    _set_if(cfg, "sub-sampling", "sub-sampling-1x1")
    _set_if(cfg, "baseline", True)
    _set_if(cfg, "dct", "float")
    _set_if(cfg, "use-original-quality", False)
    result = proc.run(cfg)
    status = result.index(0) if result else None
    return str(status)

def export_png(image, path):
    pdb = Clay.get_pdb()
    proc = pdb.lookup_procedure("file-png-export")
    if proc is None:
        Clay.file_save(Clay.RunMode.NONINTERACTIVE, image, Gio.File.new_for_path(path), None)
        return "file_save_fallback"
    cfg = proc.create_config()
    _set_if(cfg, "run-mode", Clay.RunMode.NONINTERACTIVE)
    _set_if(cfg, "image", image)
    _set_if(cfg, "file", Gio.File.new_for_path(path))
    _set_if(cfg, "interlaced", False)
    _set_if(cfg, "compression", 9)
    _set_if(cfg, "bkgd", False)
    _set_if(cfg, "offs", False)
    _set_if(cfg, "phys", True)
    _set_if(cfg, "time", True)
    _set_if(cfg, "save-transparent", True)
    result = proc.run(cfg)
    status = result.index(0) if result else None
    return str(status)

sizes = [512, 256, 128, 64, 32, 16]
saved = []
for size in sizes:
    image.scale(size, size)
    jpg = os.path.join(outdir, "clay-logo(%d).jpg" % size)
    png = os.path.join(outdir, "clay-logo(%d).png" % size)
    jstat = export_jpeg(image, jpg)
    pstat = export_png(image, png)
    saved.append({
        "size": size,
        "width": image.get_width(),
        "height": image.get_height(),
        "jpg": jpg,
        "png": png,
        "jpeg_status": jstat,
        "png_status": pstat,
    })

Clay.context_set_interpolation(prev_interp)
Clay.displays_flush()
RESULT = {
    "prev_interpolation": int(prev_interp),
    "used_interpolation": "LOHALO",
    "dpi": [xres, yres],
    "final_size": [image.get_width(), image.get_height()],
    "saved": saved,
}
raise RuntimeError("LOGO_SCALE_OK " + repr(RESULT))
'''


def call(method, params=None, timeout=180):
    payload = json.dumps({"jsonrpc": "2.0", "id": 1, "method": method, "params": params or {}}) + "\n"
    with socket.create_connection((HOST, PORT), timeout=timeout) as sock:
        sock.settimeout(timeout)
        sock.sendall(payload.encode("utf-8"))
        chunks: list[bytes] = []
        while True:
            data = sock.recv(65536)
            if not data:
                break
            chunks.append(data)
            if b"\n" in data:
                break
    reply = json.loads(b"".join(chunks).decode("utf-8").strip().splitlines()[-1])
    if "error" in reply:
        raise RuntimeError(reply["error"].get("message", reply["error"]))
    return reply.get("result")


def build_icos(outdir: Path) -> dict:
    from PIL import Image

    def load(size: int) -> Image.Image:
        png = outdir / f"clay-logo({size}).png"
        jpg = outdir / f"clay-logo({size}).jpg"
        path = png if png.exists() else jpg
        im = Image.open(path).convert("RGBA")
        if im.size != (size, size):
            im = im.resize((size, size), Image.Resampling.LANCZOS)
        return im

    imgs = {s: load(s) for s in (256, 128, 64, 32, 16)}
    imgs[48] = imgs[64].resize((48, 48), Image.Resampling.LANCZOS)

    win_ico = outdir / "clay-logo.ico"
    imgs[256].save(
        win_ico,
        format="ICO",
        sizes=[(256, 256), (128, 128), (64, 64), (48, 48), (32, 32), (16, 16)],
        append_images=[imgs[128], imgs[64], imgs[48], imgs[32], imgs[16]],
    )

    tiny_ico = outdir / "clay-logo(16).ico"
    imgs[16].save(tiny_ico, format="ICO", sizes=[(16, 16)])

    fav_png = outdir / "favicon.png"
    imgs[32].save(fav_png, format="PNG")

    fav_ico = outdir / "favicon.ico"
    imgs[32].save(
        fav_ico,
        format="ICO",
        sizes=[(48, 48), (32, 32), (16, 16)],
        append_images=[imgs[48], imgs[16]],
    )

    return {
        "windows_ico": str(win_ico),
        "ico_16": str(tiny_ico),
        "favicon_png": str(fav_png),
        "favicon_ico": str(fav_ico),
    }


def main() -> int:
    if not SRC.exists():
        print("missing source", SRC, file=sys.stderr)
        return 1
    ping = call("ping", timeout=10)
    print("ping", ping)
    try:
        call("execute_python", {"code": CLAY_CODE}, timeout=180)
        print("unexpected success without RESULT")
        clay_result = {}
    except RuntimeError as exc:
        text = str(exc)
        if "LOGO_SCALE_OK" not in text:
            raise
        clay_result = eval(text.split("LOGO_SCALE_OK ", 1)[1], {"Error": Exception})
        print("clay", json.dumps(clay_result, indent=2))
    icos = build_icos(OUT)
    print("icos", json.dumps(icos, indent=2))
    for p in list(OUT.glob("clay-logo(*).*")) + [OUT / "clay-logo.ico", OUT / "favicon.png", OUT / "favicon.ico"]:
        if p.exists():
            print(f"{p.name:24} {p.stat().st_size:8d} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
