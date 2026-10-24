import json
import socket
import sys

# White square on g7_2_landscape (~1376x768); override if needed.
# White sticker + failed fill smudge above it
WHITE_RECT = [472, 376, 132, 162]


def call(method, params=None, timeout=180):
    payload = json.dumps({"jsonrpc": "2.0", "id": 1, "method": method, "params": params or {}}) + "\n"
    print("→ %s (timeout %ss) …" % (method, timeout), flush=True)
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


def main():
    try:
        print("ping:", call("ping", timeout=10), flush=True)
    except OSError as e:
        print("Bridge not reachable:", e, file=sys.stderr)
        print("Start Tools → AI Agent Bridge in Clay.", file=sys.stderr)
        sys.exit(1)
    except TimeoutError:
        print(
            "Bridge busy (still working on last command). "
            "Click Stop on the bridge, restart it, then run this again.",
            file=sys.stderr,
        )
        sys.exit(1)

    print("fill:", call("content_aware_fill", {"rect": WHITE_RECT}, timeout=180), flush=True)
    print("snap:", call("get_canvas_snapshot", timeout=60), flush=True)
    print("done.", flush=True)


if __name__ == "__main__":
    main()
