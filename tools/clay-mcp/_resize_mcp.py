import json
import socket
import sys

def call(method, params=None, timeout=60):
    payload = json.dumps({"jsonrpc": "2.0", "id": 1, "method": method, "params": params or {}}) + "\n"
    with socket.create_connection(("127.0.0.1", 9877), timeout=timeout) as sock:
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

if __name__ == "__main__":
    try:
        print("ping:", call("ping", timeout=10), flush=True)
        r = call("resize_image", {"width": 150, "height": 150, "dpi": 88}, timeout=60)
        print("resize:", r, flush=True)
        print("state:", call("get_session_state", timeout=10), flush=True)
    except OSError as e:
        print("Bridge not reachable:", e, file=sys.stderr)
        sys.exit(1)
