import json, socket
p = json.dumps({"jsonrpc": "2.0", "id": 1, "method": "get_session_state", "params": {}}) + "\n"
s = socket.create_connection(("127.0.0.1", 9877), timeout=10)
s.sendall(p.encode())
r = json.loads(s.recv(50000).decode().strip().splitlines()[-1])
for line in r["result"]["log"]:
    print(line)
