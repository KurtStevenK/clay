import json
import socket

code = r"""
info = {}
proc = Clay.get_pdb().lookup_procedure('plug-in-inpaint-heal')
cfg = proc.create_config()
info['has_set'] = hasattr(cfg, 'set_core_object_array')
# pick landscape if present
imgs = Clay.get_images() or []
im = None
for cand in imgs:
    if cand.get_width() > 400:
        im = cand
        break
im = im or (imgs[0] if imgs else None)
dr = im.get_selected_drawables()[0] if im and im.get_selected_drawables() else (im.get_layers()[0] if im else None)
info['image'] = im.get_name() if im else None
info['size'] = [im.get_width(), im.get_height()] if im else None

def try_call(label, fn):
    try:
        fn()
        info[label] = 'ok'
    except Exception as e:
        info[label] = type(e).__name__ + ': ' + str(e)

if dr is not None:
    try_call('list', lambda: cfg.set_core_object_array('drawables', [dr]))
    try_call('list_n', lambda: cfg.set_core_object_array('drawables', [dr], 1))
    try_call('tuple', lambda: cfg.set_core_object_array('drawables', (dr,)))
    try_call('draw_only', lambda: cfg.set_core_object_array('drawables', dr))
    try_call('gegl_inpaint', lambda: (None if Gegl.has_operation('gegl:inpaint') else (_ for _ in ()).throw(RuntimeError('no'))))
    info['gegl_inpaint'] = Gegl.has_operation('gegl:inpaint') if hasattr(Gegl, 'has_operation') else 'n/a'
raise RuntimeError(repr(info))
"""
p = json.dumps({"jsonrpc": "2.0", "id": 1, "method": "execute_python", "params": {"code": code}}) + "\n"
s = socket.create_connection(("127.0.0.1", 9877), timeout=20)
s.sendall(p.encode())
print(s.recv(65536).decode())
