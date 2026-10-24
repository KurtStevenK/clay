import json, socket
s = socket.create_connection(("127.0.0.1", 9877), timeout=5)
s.settimeout(5)
s.sendall(b'{"jsonrpc":"2.0","id":1,"method":"ping","params":{}}\n')
print(s.recv(4096).decode())
