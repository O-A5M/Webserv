#!/usr/bin/env python3
"""
stress_test.py — Stress / edge-case tester for a custom webserv (nginx-like) implementation.

Usage:
    python3 stress_test.py --host 127.0.0.1 --port 8080 [--test all|flood|keepalive|slowloris|malformed|chunked|upload|fd|cgi]

Notes:
  - Uses raw sockets (stdlib only) so it can send deliberately malformed / partial
    requests that curl or ApacheBench won't let you construct.
  - Run your server under valgrind in one terminal, this script in another:
        valgrind --leak-check=full --track-fds=yes ./webserv config.conf
  - Watch for fd growth with:  watch -n1 'ls /proc/$(pgrep webserv)/fd | wc -l'
"""

import argparse
import socket
import ssl
import sys
import threading
import time
import random
import string

HOST = "127.0.0.1"
PORT = 8080
TIMEOUT = 5


def connect():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(TIMEOUT)
    s.connect((HOST, PORT))
    return s


def recv_all(s, maxlen=65536):
    try:
        data = b""
        while len(data) < maxlen:
            chunk = s.recv(4096)
            if not chunk:
                break
            data += chunk
    except socket.timeout:
        pass
    return data


# ---------------------------------------------------------------------------
# 1. Basic flood: many concurrent GET requests
# ---------------------------------------------------------------------------
def flood_worker(n, path="/"):
    ok = fail = 0
    for _ in range(n):
        try:
            s = connect()
            req = f"GET {path} HTTP/1.1\r\nHost: {HOST}\r\nConnection: close\r\n\r\n"
            s.sendall(req.encode())
            resp = recv_all(s)
            s.close()
            if resp.startswith(b"HTTP/1.1"):
                ok += 1
            else:
                fail += 1
        except Exception:
            fail += 1
    return ok, fail


def test_flood(threads=20, per_thread=50, path="/"):
    print(f"[flood] {threads} threads x {per_thread} requests to {path}")
    results = []

    def run():
        results.append(flood_worker(per_thread, path))

    tlist = [threading.Thread(target=run) for _ in range(threads)]
    start = time.time()
    for t in tlist:
        t.start()
    for t in tlist:
        t.join()
    elapsed = time.time() - start
    ok = sum(r[0] for r in results)
    fail = sum(r[1] for r in results)
    total = ok + fail
    print(f"  done in {elapsed:.2f}s | ok={ok} fail={fail} | {total/elapsed:.1f} req/s")


# ---------------------------------------------------------------------------
# 2. Keep-alive: reuse one connection for many requests
# ---------------------------------------------------------------------------
def test_keepalive(n=200, path="/"):
    print(f"[keepalive] {n} sequential requests on ONE connection")
    s = connect()
    ok = fail = 0
    for i in range(n):
        try:
            req = f"GET {path} HTTP/1.1\r\nHost: {HOST}\r\n\r\n"
            s.sendall(req.encode())
            resp = s.recv(65536)
            if resp.startswith(b"HTTP/1.1"):
                ok += 1
            else:
                fail += 1
        except Exception as e:
            fail += 1
            print(f"  broke at request {i}: {e}")
            s = connect()
    s.close()
    print(f"  ok={ok} fail={fail}")


# ---------------------------------------------------------------------------
# 3. Slowloris: dribble a request in one byte at a time to test read timeouts
# ---------------------------------------------------------------------------
def test_slowloris(n_conns=30, delay=1.0, hold=30):
    print(f"[slowloris] opening {n_conns} connections, sending 1 byte every {delay}s for {hold}s")
    socks = []
    for _ in range(n_conns):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(TIMEOUT)
            s.connect((HOST, PORT))
            s.sendall(b"G")
            socks.append(s)
        except Exception:
            pass
    print(f"  {len(socks)} connections established, trickling bytes...")
    start = time.time()
    line = b"ET / HTTP/1.1\r\nHost: x\r\nX-Pad: " + b"a" * 1 + b"\r\n"
    idx = 0
    while time.time() - start < hold:
        alive = []
        for s in socks:
            try:
                if idx < len(line):
                    s.sendall(line[idx:idx + 1])
                alive.append(s)
            except Exception:
                pass
        socks = alive
        idx += 1
        time.sleep(delay)
    print(f"  {len(socks)} connections still alive after {hold}s — server should be timing these out")
    for s in socks:
        try:
            s.close()
        except Exception:
            pass


# ---------------------------------------------------------------------------
# 4. Malformed requests — check parser doesn't crash / leak on bad input
# ---------------------------------------------------------------------------
MALFORMED_REQUESTS = [
    b"GET / HTTP/1.1\r\n\r\n",                                  # missing Host
    b"GET  / HTTP/1.1\r\nHost: x\r\n\r\n",                      # double space
    b"GETT / HTTP/1.1\r\nHost: x\r\n\r\n",                      # unknown method
    b"GET / HTTP/9.9\r\nHost: x\r\n\r\n",                       # bad version
    b"GET /../../../../etc/passwd HTTP/1.1\r\nHost: x\r\n\r\n", # traversal
    b"GET / HTTP/1.1\r\nHost:\r\n\r\n",                         # empty host value
    b"GET / HTTP/1.1\r\nContent-Length: -5\r\nHost: x\r\n\r\n", # negative length
    b"GET / HTTP/1.1\r\nContent-Length: abc\r\nHost: x\r\n\r\n",# non-numeric length
    b" GET / HTTP/1.1\r\nHost: x\r\n\r\n",                      # leading space
    b"GET " + b"/a" * 5000 + b" HTTP/1.1\r\nHost: x\r\n\r\n",   # very long URI
    b"\r\n\r\nGET / HTTP/1.1\r\nHost: x\r\n\r\n",               # leading CRLF junk
    b"",                                                        # empty / immediate close
]


def test_malformed():
    print(f"[malformed] sending {len(MALFORMED_REQUESTS)} bad requests, one per connection")
    for i, req in enumerate(MALFORMED_REQUESTS):
        try:
            s = connect()
            if req:
                s.sendall(req)
            resp = recv_all(s)
            status = resp.split(b"\r\n")[0] if resp else b"(no response)"
            print(f"  [{i}] {status}")
            s.close()
        except Exception as e:
            print(f"  [{i}] exception: {e}")


# ---------------------------------------------------------------------------
# 5. Chunked transfer-encoding POST
# ---------------------------------------------------------------------------
def test_chunked():
    print("[chunked] sending a chunked POST body")
    s = connect()
    header = (
        f"POST /uploads/ HTTP/1.1\r\nHost: {HOST}\r\n"
        "Transfer-Encoding: chunked\r\nContent-Type: text/plain\r\n\r\n"
    )
    body_parts = ["hello ", "chunked ", "world!"]
    payload = header.encode()
    for part in body_parts:
        data = part.encode()
        payload += f"{len(data):x}\r\n".encode() + data + b"\r\n"
    payload += b"0\r\n\r\n"
    s.sendall(payload)
    resp = recv_all(s)
    print(f"  response: {resp[:200]!r}")
    s.close()


# ---------------------------------------------------------------------------
# 6. Large multipart upload
# ---------------------------------------------------------------------------
def test_upload(size_mb=5):
    print(f"[upload] multipart upload of ~{size_mb}MB")
    boundary = "----WebservStressBoundary"
    filename = "".join(random.choices(string.ascii_lowercase, k=8)) + ".bin"
    filedata = bytes(random.getrandbits(8) for _ in range(1024)) * (size_mb * 1024)

    body = (
               f"--{boundary}\r\n"
               f'Content-Disposition: form-data; name="file"; filename="{filename}"\r\n'
               "Content-Type: application/octet-stream\r\n\r\n"
           ).encode() + filedata + f"\r\n--{boundary}--\r\n".encode()

    header = (
        f"POST /uploads/ HTTP/1.1\r\nHost: {HOST}\r\n"
        f"Content-Type: multipart/form-data; boundary={boundary}\r\n"
        f"Content-Length: {len(body)}\r\nConnection: close\r\n\r\n"
    ).encode()

    s = connect()
    s.sendall(header + body)
    resp = recv_all(s, maxlen=4096)
    print(f"  response: {resp[:200]!r}")
    s.close()


# ---------------------------------------------------------------------------
# 7. FD-leak hunter: rapid connect + abrupt disconnect (no request sent)
# ---------------------------------------------------------------------------
def test_fd_leak(n=500):
    print(f"[fd] opening/closing {n} connections abruptly (no request, or partial request)")
    for i in range(n):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(1)
            s.connect((HOST, PORT))
            if i % 3 == 0:
                s.sendall(b"GET / HTTP/1.1\r\n")  # partial, then vanish
            s.close()  # abrupt close, no full request/response cycle
        except Exception:
            pass
    print("  done — check server fd count now (lsof -p <pid> | wc -l) and after a pause")


# ---------------------------------------------------------------------------
# 8. Concurrent CGI hits
# ---------------------------------------------------------------------------
def test_cgi(path="/cgi-bin/test.py", n=50, threads=10):
    print(f"[cgi] {threads} threads x {n} requests to {path}")
    test_flood(threads=threads, per_thread=n, path=path)


ALL_TESTS = {
    "flood": lambda: test_flood(),
    "keepalive": lambda: test_keepalive(),
    "slowloris": lambda: test_slowloris(),
    "malformed": lambda: test_malformed(),
    "chunked": lambda: test_chunked(),
    "upload": lambda: test_upload(),
    "fd": lambda: test_fd_leak(),
    "cgi": lambda: test_cgi(),
}


def main():
    global HOST, PORT
    parser = argparse.ArgumentParser(description="Stress test a webserv instance")
    parser.add_argument("--host", default=HOST)
    parser.add_argument("--port", type=int, default=PORT)
    parser.add_argument("--test", default="all", choices=["all"] + list(ALL_TESTS.keys()))
    args = parser.parse_args()

    HOST = args.host
    PORT = args.port

    print(f"Target: {HOST}:{PORT}\n")

    if args.test == "all":
        for name, fn in ALL_TESTS.items():
            print(f"\n=== {name} ===")
            try:
                fn()
            except Exception as e:
                print(f"  test '{name}' raised: {e}")
    else:
        ALL_TESTS[args.test]()


if __name__ == "__main__":
    main()