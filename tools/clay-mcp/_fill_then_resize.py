"""Step 1: content-aware fill white box. Step 2: resize 150x150 @ 88dpi."""
import json
import socket
import sys
import time

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


def wait_bridge():
    for _ in range(45):
        try:
            call("ping", timeout=3)
            return
        except (OSError, TimeoutError, RuntimeError):
            time.sleep(2)
    raise SystemExit("MCP bridge not running. Start Tools → AI Agent Bridge in Clay.")


def main():
    wait_bridge()
    print("1/2 content_aware_fill …", flush=True)
    print(call("content_aware_fill", {"auto_select_white": True}), flush=True)
    print("2/2 resize_image …", flush=True)
    print(call("resize_image", {"width": 150, "height": 150, "dpi": 88}), flush=True)
    print("done:", call("get_session_state", timeout=10), flush=True)


if __name__ == "__main__":
    main()
