import json
import socket

code = """
pdb = Clay.get_pdb()
scale = pdb.lookup_procedure('clay-image-scale')
cfg = scale.create_config()
cfg.set_property('image', image)
cfg.set_property('new-width', 150)
cfg.set_property('new-height', 150)
scale.run(cfg)
res = pdb.lookup_procedure('clay-image-set-resolution')
cfg2 = res.create_config()
cfg2.set_property('image', image)
cfg2.set_property('xresolution', 88.0)
cfg2.set_property('yresolution', 88.0)
res.run(cfg2)
result = {'w': image.get_width(), 'h': image.get_height()}
"""

payload = json.dumps({
    "jsonrpc": "2.0", "id": 1,
    "method": "execute_python",
    "params": {"code": code},
}) + "\n"
with socket.create_connection(("127.0.0.1", 9877), timeout=60) as s:
    s.sendall(payload.encode())
    print(s.recv(65536).decode())
payload2 = json.dumps({"jsonrpc": "2.0", "id": 2, "method": "get_session_state", "params": {}}) + "\n"
with socket.create_connection(("127.0.0.1", 9877), timeout=10) as s:
    s.sendall(payload2.encode())
    r = json.loads(s.recv(65536).decode().strip().splitlines()[-1])
    print("session:", r.get("result"))
