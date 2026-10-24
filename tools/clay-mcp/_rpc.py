import json, socket, sys, time


def zoom_clay_100():
    """Focus Clay and send the 1:1 shortcut (View → Zoom 1:1 / 100%)."""
    try:
        import ctypes
        user32 = ctypes.windll.user32
        hwnd = user32.FindWindowW(None, "Clay Image Beautification Program")
        if not hwnd:
            return False
        user32.ShowWindow(hwnd, 9)
        user32.SetForegroundWindow(hwnd)
        time.sleep(0.25)
        user32.keybd_event(0x31, 0, 0, 0)
        user32.keybd_event(0x31, 0, 2, 0)
        return True
    except Exception:
        return False


def clay(method, params=None, timeout=90):
    p = json.dumps({"jsonrpc": "2.0", "id": 1, "method": method, "params": params or {}}) + "\n"
    s = socket.create_connection(("127.0.0.1", 9877), timeout=timeout)
    s.sendall(p.encode())
    chunks = []
    while True:
        data = s.recv(65536)
        if not data:
            break
        chunks.append(data)
        if b"\n" in data:
            break
    raw = b"".join(chunks).decode().strip()
    reply = json.loads(raw.splitlines()[-1])
    if "error" in reply:
        raise RuntimeError(reply["error"].get("message", str(reply["error"])))
    result = reply.get("result")
    if method in ("open_image", "create_image"):
        zoom_clay_100()
    return result

if __name__ == "__main__":
    method = sys.argv[1]
    params = json.loads(sys.argv[2]) if len(sys.argv) > 2 else {}
    print(json.dumps(clay(method, params), indent=2))
