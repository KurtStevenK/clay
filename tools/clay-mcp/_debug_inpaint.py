import json
import socket

code = """
pdb = Clay.get_pdb()
proc = pdb.lookup_procedure('plug-in-inpaint-heal')
cfg = proc.create_config()
cfg.set_property('run-mode', Clay.RunMode.NONINTERACTIVE)
cfg.set_property('image', image)
cfg.set_property('drawables', Clay.ObjectArray.new(Clay.Drawable, drawable, False))
cfg.set_property('sampling-radius', 12)
cfg.set_property('refinement-passes', 2)
cfg.set_property('heal-transparency', False)
r = proc.run(cfg)
raise RuntimeError('status=%s' % r.index(0))
"""

payload = json.dumps({"jsonrpc": "2.0", "id": 1, "method": "execute_python", "params": {"code": code}}) + "\n"
s = socket.create_connection(("127.0.0.1", 9877), timeout=120)
s.sendall(payload.encode())
print(s.recv(65536).decode())
