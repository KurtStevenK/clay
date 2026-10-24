#!/usr/bin/env python3
"""Full /goal demo: open image, rect select, white fill, content-aware, 50% scale."""
import json
import socket
import sys
import time

IMAGE = r"C:\Users\Kurt Steven\Pictures\essen\g7_2_landscape.jpg"
HOST, PORT = "127.0.0.1", 9877


def call(method, params=None, timeout=90):
    payload = json.dumps({"jsonrpc": "2.0", "id": 1, "method": method, "params": params or {}}) + "\n"
    with socket.create_connection((HOST, PORT), timeout=timeout) as sock:
        sock.settimeout(timeout)
        sock.sendall(payload.encode("utf-8"))
        chunks = []
        while True:
            data = sock.recv(65536)
            if not data:
                break
            chunks.append(data)
            if b"\n" in data:
                break
    reply = json.loads(b"".join(chunks).decode().strip().splitlines()[-1])
    if "error" in reply:
        raise RuntimeError(reply["error"].get("message", reply["error"]))
    return reply.get("result")


def wait_bridge(seconds=120):
    for i in range(seconds):
        try:
            return call("ping", timeout=5)
        except OSError:
            time.sleep(1)
    raise SystemExit("MCP bridge not up — start Clay with CLAY_MCP_AUTOSTART=1")


def main():
    print("ping:", wait_bridge())
    print("open:", call("open_image", {"path": IMAGE}))
    state = call("get_session_state")
    im = state["images"][0]
    w, h = im["width"], im["height"]
    print("size:", w, h)
    rw, rh = 160, 120
    rx, ry = w // 2 - rw // 2, h // 2 - rh // 2
    print("select:", call("manage_layers_and_selections", {
        "action": "select_rectangle",
        "rect": [rx, ry, rw, rh],
    }))
    fill_code = """
white = Gegl.Color()
white.set_rgba(1.0, 1.0, 1.0, 1.0)
Clay.context_set_foreground(white)
image.undo_group_start()
drawable.edit_fill(Clay.FillType.FOREGROUND)
image.undo_group_end()
"""
    print("white:", call("execute_python", {"code": fill_code}))
    print("heal:", call("content_aware_fill", {
        "auto_select_white": False,
        "use_inpaint_plugin": True,
        "sampling_radius": 6,
        "refinement_passes": 1,
    }, timeout=120))
    print("resize:", call("resize_image", {"scale_percent": 50}))
    snap = call("get_canvas_snapshot")
    print("snapshot:", snap)
    final = call("get_session_state")
    print("final:", final["images"][0])


if __name__ == "__main__":
    main()
