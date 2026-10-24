import json
import socket

code = """
image = Clay.get_images()[0]
drawable = image.get_layers()[0]
w, h = image.get_width(), image.get_height()
image.select_rectangle(Clay.ChannelOps.REPLACE, 300, 140, 90, 70)
Clay.edit_copy([drawable])
image.select_rectangle(Clay.ChannelOps.REPLACE, 450, 140, 90, 70)
layers = Clay.edit_paste(drawable, True)
fs = image.get_floating_sel()
raise RuntimeError('paste=%s fs=%s anchor=%s' % (layers, fs, hasattr(Clay, 'floating_sel_anchor')))
"""
payload = json.dumps({"jsonrpc": "2.0", "id": 1, "method": "execute_python", "params": {"code": code}}) + "\n"
s = socket.create_connection(("127.0.0.1", 9877), timeout=30)
s.sendall(payload.encode())
print(s.recv(4096).decode())
