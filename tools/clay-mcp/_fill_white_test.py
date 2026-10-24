#!/usr/bin/env python3
"""Find white bbox (fast ROI scan) and heal via Gegl buffer."""
import json
import socket
import sys

HOST = "127.0.0.1"
PORT = 9877

HEAL_CODE = r"""
from gi.repository import Gegl, Babl

Gegl.init(None)
TH = 0.92
w, h = drawable.get_width(), drawable.get_height()
buf = drawable.get_buffer()
fmt = Babl.format("R'G'B'A u8")

# Likely table area only (fast scan).
rx0, ry0, rw0, rh0 = 280, 320, 720, 420
roi = Gegl.Rectangle()
roi.x, roi.y, roi.width, roi.height = rx0, ry0, rw0, rh0
raw = buf.get(roi, 1.0, fmt, Gegl.AbyssPolicy.CLAMP)
stride = rw0 * 4

def near_white_at(lx, ly):
    i = (ly * rw0 + lx) * 4
    r, g, b, a = raw[i], raw[i + 1], raw[i + 2], raw[i + 3]
    return r > int(TH * 255) and g > int(TH * 255) and b > int(TH * 255) and a > 128

minx, miny, maxx, maxy = w, h, -1, -1
n = 0
for ly in range(rh0):
    for lx in range(rw0):
        if near_white_at(lx, ly):
            n += 1
            gx, gy = rx0 + lx, ry0 + ly
            minx = min(minx, gx)
            miny = min(miny, gy)
            maxx = max(maxx, gx)
            maxy = max(maxy, gy)

if n < 20 or maxx < 0:
    raise RuntimeError("No white patch in ROI (%d px)" % n)

# Tighten to connected component around center of bbox (drop stray bright pixels).
cx = (minx + maxx) // 2
cy = (miny + maxy) // 2
rx, ry = minx, miny
rw, rh = maxx - minx + 1, maxy - miny + 1

full_roi = Gegl.Rectangle()
full_roi.x, full_roi.y, full_roi.width, full_roi.height = 0, 0, w, h
pixels = bytearray(buf.get(full_roi, 1.0, fmt, Gegl.AbyssPolicy.CLAMP))

def idx(x, y):
    return (y * w + x) * 4

def is_white(x, y):
    if not (rx <= x < rx + rw and ry <= y < ry + rh):
        return False
    i = idx(x, y)
    r, g, b, a = pixels[i], pixels[i + 1], pixels[i + 2], pixels[i + 3]
    return r > int(TH * 255) and g > int(TH * 255) and b > int(TH * 255) and a > 128

image.undo_group_start()
Clay.Selection.none(image)
image.select_rectangle(Clay.ChannelOps.REPLACE, rx, ry, rw, rh)

for _ in range(300):
    changed = False
    for y in range(ry, ry + rh):
        for x in range(rx, rx + rw):
            if not is_white(x, y):
                continue
            rs = gs = bs = 0
            cnt = 0
            for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h and not is_white(nx, ny):
                    i = idx(nx, ny)
                    rs += pixels[i]
                    gs += pixels[i + 1]
                    bs += pixels[i + 2]
                    cnt += 1
            if cnt:
                i = idx(x, y)
                pixels[i] = rs // cnt
                pixels[i + 1] = gs // cnt
                pixels[i + 2] = bs // cnt
                changed = True
    if not changed:
        break

buf.set(full_roi, pixels, fmt)
buf.flush()
del buf
Clay.Selection.none(image)
image.undo_group_end()
Clay.displays_flush()
"""


def call(method, params=None, timeout=300):
    payload = json.dumps(
        {"jsonrpc": "2.0", "id": 1, "method": method, "params": params or {}}
    ) + "\n"
    with socket.create_connection((HOST, PORT), timeout=timeout) as sock:
        sock.sendall(payload.encode("utf-8"))
        chunks = []
        while True:
            data = sock.recv(65536)
            if not data:
                break
            chunks.append(data)
            if b"\n" in data:
                break
    line = b"".join(chunks).decode("utf-8").strip().splitlines()[-1]
    reply = json.loads(line)
    if "error" in reply:
        raise RuntimeError(reply["error"].get("message", reply["error"]))
    return reply.get("result")


def main():
    print("session:", call("get_session_state", timeout=10))
    call("execute_python", {"code": HEAL_CODE}, timeout=300)
    print("heal: ok")
    print("snapshot:", call("get_canvas_snapshot", timeout=60))


if __name__ == "__main__":
    main()
