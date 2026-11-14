#!/usr/bin/env python3
"""Send the firmware's exact GET request to a server and save the raw reply.

    python3 tests/host/capture.py api.coingecko.com "/api/v3/simple/price?ids=bitcoin&vs_currencies=usd" reply.bin
    tests/host/run.sh reply.bin     # decode it with the firmware's reader

The User-Agent's board field is DEPLOY-TEST so the update checker's statistics can leave these requests out.
"""
import socket, ssl, sys

host, path, out = sys.argv[1], sys.argv[2], sys.argv[3]
ctx = ssl.create_default_context()
ctx.check_hostname = False
ctx.verify_mode = ssl.CERT_NONE  # like the firmware's setInsecure()
request = (f"GET {path} HTTP/1.1\r\nHost: {host}\r\n"
           "User-Agent: 6.4.0|DEPLOY-TEST|DEPG0213BN|Jun 22 2026 16:52:09\r\n"
           "Content-Type: application/json\r\nConnection: close\r\n\r\n")
with socket.create_connection((host, 443), timeout=15) as raw, ctx.wrap_socket(raw, server_hostname=host) as tls:
    tls.sendall(request.encode())
    data = b""
    while True:
        piece = tls.recv(4096)
        if not piece:
            break
        data += piece
open(out, "wb").write(data)
print(f"{len(data)} bytes saved to {out}")
