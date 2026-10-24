#!/usr/bin/env python3
"""Random white patch + high-quality content-aware fill (no resize)."""
import json
import socket
import sys
import time

IMAGE = r"C:\Users\Kurt Steven\Pictures\essen\g7_2_landscape.jpg"
HOST, PORT = "127.0.0.1", 9877


def call(method, params=None, timeout=180):
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


def wait_bridge(seconds=90):
    for _ in range(seconds):
        try:
            return call("ping", timeout=5)
        except OSError:
            time.sleep(1)
    raise SystemExit("Start Clay (CLAY_MCP_AUTOSTART=1) or AI Agent Bridge.")


def main():
    print("ping:", wait_bridge())
    print("open:", call("open_image", {"path": IMAGE}))
    call("manage_layers_and_selections", {"action": "none"})
    print("heal:", call("content_aware_fill", {
        "make_random_white_patch": True,
        "white_box_surface": "table",
        "auto_select_white": False,
    }, timeout=180))
    snap = call("get_canvas_snapshot")
    print("snapshot:", snap)
    print("log:", call("get_session_state")["log"][-10:])


if __name__ == "__main__":
    main()
