"""Wait for MCP bridge, open snapshot if needed, fill white rect."""
import json
import socket
import subprocess
import sys
import time

HOST = "127.0.0.1"
PORT = 9877
WHITE_RECT = [472, 376, 132, 162]
SNAPSHOT = r"C:\Users\Kurt Steven\.cursor\projects\t-Apps-clay\assets\c__Users_Kurt_Steven_AppData_Roaming_Cursor_User_workspaceStorage_75c95bc2c412a9c63b349f887235c830_images_image-2f389232-cf51-4990-9807-52b02be47a20.png"


def call(method, params=None, timeout=120):
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


def port_open():
    try:
        call("ping", timeout=3)
        return True
    except (OSError, TimeoutError):
        return False


def main():
    print("Waiting for bridge on %s:%s …" % (HOST, PORT), flush=True)
    for i in range(90):
        if port_open():
            print("Bridge up after %ds." % i, flush=True)
            break
        time.sleep(2)
    else:
        print("Bridge did not start. Open Tools → AI Agent Bridge in Clay.", file=sys.stderr)
        sys.exit(1)

    state = call("get_session_state", timeout=10)
    print("session:", state, flush=True)
    if state.get("image_count", 0) < 1:
        import os
        if os.path.isfile(SNAPSHOT):
            print("opening snapshot …", flush=True)
            call("open_image", {"path": SNAPSHOT}, timeout=30)
        else:
            print("No image open — open g7_2_landscape in Clay and re-run.", file=sys.stderr)
            sys.exit(1)

    print("filling …", flush=True)
    print(call("content_aware_fill", {"rect": WHITE_RECT}, timeout=120), flush=True)
    print("snapshot:", call("get_canvas_snapshot", timeout=60), flush=True)
    print("done.", flush=True)


if __name__ == "__main__":
    main()
