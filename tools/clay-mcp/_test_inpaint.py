import json
import socket

code = r"""
from gi.repository import Gio
im = max(Clay.get_images(), key=lambda i: i.get_id())
dr = im.get_layers()[0]
rx, ry, rw, rh = 372, 148, 86, 58
im.select_rectangle(Clay.ChannelOps.REPLACE, rx, ry, rw, rh)
white = Gegl.Color()
white.set_rgba(1.0, 1.0, 1.0, 1.0)
Clay.context_set_foreground(white)
dr.edit_fill(Clay.FillType.FOREGROUND)
Clay.Selection.none(im)
before = r'T:\Apps\clay\tools\clay-mcp\inpaint-test-before.png'
dup = im.duplicate()
Clay.file_save(Clay.RunMode.NONINTERACTIVE, dup, Gio.File.new_for_path(before), None)
dup.delete()
im.select_rectangle(Clay.ChannelOps.REPLACE, rx, ry, rw, rh)
proc = Clay.get_pdb().lookup_procedure('plug-in-inpaint-heal')
cfg = proc.create_config()
cfg.set_property('run-mode', Clay.RunMode.NONINTERACTIVE)
cfg.set_property('image', im)
cfg.set_core_object_array('drawables', [dr])
cfg.set_property('sampling-radius', 28)
cfg.set_property('refinement-passes', 5)
cfg.set_property('heal-transparency', False)
res = proc.run(cfg)
status = int(res.index(0))
Clay.Selection.none(im)
after = r'T:\Apps\clay\tools\clay-mcp\inpaint-test-after.png'
dup = im.duplicate()
Clay.file_save(Clay.RunMode.NONINTERACTIVE, dup, Gio.File.new_for_path(after), None)
dup.delete()
Clay.displays_flush()
raise RuntimeError('CLEAN_INPAINT status=%s id=%s' % (status, im.get_id()))
"""
p = json.dumps({"jsonrpc": "2.0", "id": 1, "method": "execute_python", "params": {"code": code}}) + "\n"
s = socket.create_connection(("127.0.0.1", 9877), timeout=90)
s.sendall(p.encode())
print(s.recv(65536).decode())
