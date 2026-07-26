#!/usr/bin/env python3
"""
webserv stress test suite
==========================
Targets the specific weak points found in the codebase:
  - EPOLLIN/EPOLLOUT priority bug in EventLoop::Loop (else if instead of independent if)
  - recv()==0/-1 not calling OnClose() (leaked fds)
  - chunked transfer-encoding parser (Request::parse_body)
  - content-length vs actual body mismatches
  - multipart/form-data boundary parsing (Response::handlePost)
  - CGI fork/pipe handling under concurrency
  - MAX_URI_LENGTH / MAX_HEADER_SIZE / client_max_body_size enforcement
  - connection keep-alive / close correctness

Usage:
    python3 stress_test.py --host 127.0.0.1 --port 8080 [--test all]

Run tests individually with --test <name>, e.g.:
    python3 stress_test.py --test slowloris
    python3 stress_test.py --test fd_leak
"""

import argparse
import socket
import ssl
import threading
import time
import random
import string
import sys

HOST = "127.0.0.1"
PORT = 8080
TIMEOUT = 5


def connect():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(TIMEOUT)
    s.connect((HOST, PORT))
    return s


def recv_all(s, max_bytes=65536, timeout=2):
    s.settimeout(timeout)
    data = b""
    try:
        while len(data) < max_bytes:
            chunk = s.recv(4096)
            if not chunk:
                break
            data += chunk
    except socket.timeout:
        pass
    return data


# ---------------------------------------------------------------------------
# 1. Connection flood — many concurrent sockets held open without sending data
# ---------------------------------------------------------------------------
def test_connection_flood(n=2000):
    print(f"[connection_flood] opening {n} sockets, holding them open...")
    socks = []
    opened = 0
    failed = 0
    try:
        for i in range(n):
            try:
                s = connect()
                socks.append(s)
                opened += 1
            except Exception as e:
                failed += 1
                if failed == 1:
                    print(f"  first failure at {opened} open sockets: {e}")
                break
    finally:
        print(f"  opened={opened} failed={failed}")
        for s in socks:
            try:
                s.close()
            except Exception:
                pass
    print("  -> server should survive this without crashing; check fd count "
          "(ulimit -n) and that OnRead handles accept() failures gracefully.")


# ---------------------------------------------------------------------------
# 2. Slowloris — trickle request line/headers one byte at a time, many conns
# ---------------------------------------------------------------------------
def test_slowloris(n_conns=200, delay=0.5, duration=30):
    print(f"[slowloris] {n_conns} connections trickling headers for {duration}s...")
    stop_time = time.time() + duration
    socks = []

    def drip(sock):
        try:
            sock.send(b"GET /")
            while time.time() < stop_time:
                sock.send(random.choice(string.ascii_letters).encode())
                time.sleep(delay)
        except Exception:
            pass

    threads = []
    for _ in range(n_conns):
        try:
            s = connect()
            socks.append(s)
            t = threading.Thread(target=drip, args=(s,), daemon=True)
            t.start()
            threads.append(t)
        except Exception as e:
            print(f"  failed to open connection: {e}")
            break

    for t in threads:
        t.join()
    for s in socks:
        try:
            s.close()
        except Exception:
            pass
    print("  -> if the server has no read/header timeout, these fds stay "
          "consumed for the whole run. Check AHandler::SetTimeout is used "
          "for headers, not just after routing.")


# ---------------------------------------------------------------------------
# 3. Malformed request lines
# ---------------------------------------------------------------------------
MALFORMED_LINES = [
    b"",  # empty
    b"GET\r\n\r\n",  # no URI/version
    b"GET  /  HTTP/1.1\r\n\r\n",  # double spaces
    b" GET / HTTP/1.1\r\n\r\n",  # leading space
    b"GET / HTTP/1.1 extra\r\n\r\n",  # extra token
    b"GET /" + b"A" * 9000 + b" HTTP/1.1\r\n\r\n",  # URI > MAX_URI_LENGTH
    b"GET / HTTP/2.0\r\n\r\n",  # unsupported version
    b"FOO / HTTP/1.1\r\nHost: x\r\n\r\n",  # unknown method
    b"GET /../../../etc/passwd HTTP/1.1\r\nHost: x\r\n\r\n",  # traversal
    b"GET /%2e%2e/%2e%2e/etc/passwd HTTP/1.1\r\nHost: x\r\n\r\n",  # encoded traversal
    b"GET / HTTP/1.1\r\n\r\n\r\n\r\n",  # missing host header
    b"\r\n\r\nGET / HTTP/1.1\r\nHost: x\r\n\r\n",  # leading CRLFs
]


def test_malformed_lines():
    print("[malformed_lines] sending malformed request lines...")
    for i, payload in enumerate(MALFORMED_LINES):
        try:
            s = connect()
            s.send(payload)
            resp = recv_all(s)
            status = resp.split(b"\r\n", 1)[0] if resp else b"<no response>"
            conn_hdr = b"close" in resp.lower()
            print(f"  [{i}] sent {len(payload)}B -> {status!r} close_hdr={conn_hdr}")
            s.close()
        except Exception as e:
            print(f"  [{i}] exception: {e}")


# ---------------------------------------------------------------------------
# 4. Oversized headers
# ---------------------------------------------------------------------------
def test_oversized_headers():
    print("[oversized_headers] sending headers > MAX_HEADER_SIZE...")
    big_value = "A" * 20000
    req = f"GET / HTTP/1.1\r\nHost: x\r\nX-Big: {big_value}\r\n\r\n".encode()
    s = connect()
    s.send(req)
    resp = recv_all(s)
    print(f"  status: {resp.splitlines()[0] if resp else b'<none>'}")
    s.close()

    print("[oversized_headers] sending thousands of tiny headers...")
    hdrs = "".join(f"X-H{i}: v\r\n" for i in range(20000))
    req = f"GET / HTTP/1.1\r\nHost: x\r\n{hdrs}\r\n".encode()
    s = connect()
    try:
        s.send(req)
    except Exception as e:
        print(f"  send failed (expected if server closes early): {e}")
    resp = recv_all(s)
    print(f"  status: {resp.splitlines()[0] if resp else b'<none>'}")
    s.close()


# ---------------------------------------------------------------------------
# 5. Chunked transfer-encoding abuse — targets Request::parse_body TE branch
# ---------------------------------------------------------------------------
def test_chunked_abuse():
    print("[chunked_abuse] exercising chunked parser edge cases...")

    cases = {
        "valid": b"4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n",
        "bad_hex_size": b"ZZZ\r\ndata\r\n0\r\n\r\n",
        "negative_looking_size": b"-1\r\ndata\r\n0\r\n\r\n",
        "huge_declared_size": b"FFFFFFFF\r\n" + b"A" * 100 + b"\r\n0\r\n\r\n",
        "size_bigger_than_sent": b"100\r\nshort\r\n0\r\n\r\n",
        "missing_final_chunk": b"4\r\ntest\r\n",  # never sends 0\r\n\r\n
        "zero_chunk_with_trailer_garbage": b"0\r\nX-Trailer: evil\r\n\r\n",
        "double_terminator": b"4\r\ntest\r\n0\r\n\r\n0\r\n\r\n",
    }

    for name, body in cases.items():
        req = (
            b"POST /upload HTTP/1.1\r\n"
            b"Host: x\r\n"
            b"Transfer-Encoding: chunked\r\n"
            b"Content-Type: text/plain\r\n"
            b"\r\n" + body
        )
        try:
            s = connect()
            s.send(req)
            resp = recv_all(s, timeout=3)
            status = resp.splitlines()[0] if resp else b"<no response / hang>"
            print(f"  [{name}] -> {status}")
            s.close()
        except Exception as e:
            print(f"  [{name}] exception: {e}")


# ---------------------------------------------------------------------------
# 6. Content-Length lies
# ---------------------------------------------------------------------------
def test_content_length_lies():
    print("[content_length_lies] mismatched Content-Length vs body...")

    cases = {
        "claims_more_than_sent": (b"hello", 1000),
        "claims_less_than_sent": (b"A" * 1000, 5),
        "negative": (b"hello", None, "-5"),
        "non_numeric": (b"hello", None, "abc"),
        "zero_with_body": (b"nonempty", 0),
        "duplicate_headers": (b"hello", 5, None, True),
    }

    for name, spec in cases.items():
        body = spec[0]
        cl = spec[1] if len(spec) > 1 else None
        cl_raw = spec[2] if len(spec) > 2 else None
        dup = spec[3] if len(spec) > 3 else False

        cl_header = ""
        if cl_raw is not None:
            cl_header = f"Content-Length: {cl_raw}\r\n"
        elif cl is not None:
            cl_header = f"Content-Length: {cl}\r\n"
            if dup:
                cl_header += f"Content-Length: {cl}\r\n"

        req = (
            f"POST /upload HTTP/1.1\r\nHost: x\r\n{cl_header}"
            f"Content-Type: text/plain\r\n\r\n"
        ).encode() + body

        try:
            s = connect()
            s.send(req)
            resp = recv_all(s, timeout=3)
            status = resp.splitlines()[0] if resp else b"<no response / hang>"
            print(f"  [{name}] -> {status}")
            s.close()
        except Exception as e:
            print(f"  [{name}] exception: {e}")


# ---------------------------------------------------------------------------
# 7. Body size limit boundary (client_max_body_size)
# ---------------------------------------------------------------------------
def test_body_size_boundary(limit=1000000):
    print(f"[body_size_boundary] testing around client_max_body_size={limit}...")
    for delta, label in [(-1, "one_under"), (0, "exact"), (1, "one_over"), (10_000_000, "way_over")]:
        size = max(0, limit + delta)
        body = b"A" * size
        req = (
            f"POST /upload HTTP/1.1\r\nHost: x\r\nContent-Length: {size}\r\n"
            f"Content-Type: application/octet-stream\r\n\r\n"
        ).encode()
        try:
            s = connect()
            s.sendall(req)
            # send in chunks to avoid blocking forever on huge payloads
            sent = 0
            view = memoryview(body)
            s.settimeout(TIMEOUT)
            while sent < len(view):
                n = s.send(view[sent:sent + 65536])
                sent += n
            resp = recv_all(s, timeout=3)
            status = resp.splitlines()[0] if resp else b"<no response>"
            print(f"  [{label}] size={size} -> {status}")
            s.close()
        except Exception as e:
            print(f"  [{label}] size={size} exception: {e}")


# ---------------------------------------------------------------------------
# 8. Malformed multipart uploads — targets Response::handlePost parser
# ---------------------------------------------------------------------------
def test_malformed_multipart():
    print("[malformed_multipart] fuzzing the multipart boundary parser...")

    boundary = "----WebKitBoundaryXYZ"

    cases = {
        "no_boundary_in_body": (
            f"--{boundary}\r\nContent-Disposition: form-data; name=\"f\"; filename=\"a.txt\"\r\n\r\n"
            f"hello world\r\n--WRONGBOUNDARY--\r\n"
        ),
        "missing_final_boundary": (
            f"--{boundary}\r\nContent-Disposition: form-data; name=\"f\"; filename=\"a.txt\"\r\n\r\n"
            f"hello world\r\n"
        ),
        "filename_path_traversal": (
            f"--{boundary}\r\nContent-Disposition: form-data; name=\"f\"; filename=\"../../etc/passwd\"\r\n\r\n"
            f"pwned\r\n--{boundary}--\r\n"
        ),
        "no_headers_no_blank_line": (
            f"--{boundary}\r\nrandom garbage no double crlf"
        ),
        "empty_body": "",
        "boundary_only": f"--{boundary}--\r\n",
        "nested_boundary_lookalike": (
            f"--{boundary}\r\nContent-Disposition: form-data; name=\"f\"; filename=\"a.txt\"\r\n\r\n"
            f"data --{boundary} not really a boundary\r\n--{boundary}--\r\n"
        ),
    }

    for name, payload in cases.items():
        payload_bytes = payload.encode()
        req = (
            f"POST /upload HTTP/1.1\r\nHost: x\r\n"
            f"Content-Type: multipart/form-data; boundary={boundary}\r\n"
            f"Content-Length: {len(payload_bytes)}\r\n\r\n"
        ).encode() + payload_bytes

        try:
            s = connect()
            s.send(req)
            resp = recv_all(s, timeout=3)
            status = resp.splitlines()[0] if resp else b"<no response / hang>"
            print(f"  [{name}] -> {status}")
            s.close()
        except Exception as e:
            print(f"  [{name}] exception: {e}")


# ---------------------------------------------------------------------------
# 9. Keep-alive pipelining abuse
# ---------------------------------------------------------------------------
def test_pipelining(n=500):
    print(f"[pipelining] sending {n} pipelined GET requests on one connection...")
    req = b"GET / HTTP/1.1\r\nHost: x\r\n\r\n"
    s = connect()
    s.sendall(req * n)
    resp = recv_all(s, max_bytes=2_000_000, timeout=5)
    count = resp.count(b"HTTP/1.1")
    print(f"  sent={n} responses_seen~={count} bytes_received={len(resp)}")
    s.close()


# ---------------------------------------------------------------------------
# 10. Abrupt disconnects mid-request (tests recv()==0/-1 handling -> fd leak)
# ---------------------------------------------------------------------------
def test_fd_leak(rounds=1000):
    print(f"[fd_leak] opening+abruptly-closing {rounds} half-sent connections...")
    for i in range(rounds):
        try:
            s = connect()
            s.send(b"GET / HTTP/1.1\r\nHost: x\r\n")  # never finish headers
            # hard reset instead of graceful close/shutdown
            s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, b"\x01\x00\x00\x00")
            s.close()
        except Exception as e:
            print(f"  round {i}: {e}")
            break
    print("  -> if OnClose() isn't called on recv()==0/-1, fds/handlers leak "
          "here. Watch `lsof -p <pid> | wc -l` on the server during this test.")


# ---------------------------------------------------------------------------
# 11. Concurrent CGI stress
# ---------------------------------------------------------------------------
def test_cgi_concurrency(path="/cgi-bin/test.py", n=100):
    print(f"[cgi_concurrency] firing {n} concurrent CGI requests at {path}...")
    results = []
    lock = threading.Lock()

    def worker(i):
        try:
            s = connect()
            req = f"GET {path} HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n".encode()
            s.send(req)
            resp = recv_all(s, timeout=10)
            status = resp.splitlines()[0] if resp else b"<no response / hang>"
            with lock:
                results.append((i, status))
            s.close()
        except Exception as e:
            with lock:
                results.append((i, f"exception: {e}"))

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(n)]
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=15)

    ok = sum(1 for _, s in results if isinstance(s, bytes) and s.startswith(b"HTTP/1.1 2"))
    print(f"  completed={len(results)}/{n} ok={ok}")
    print("  -> check for zombie processes (ps aux | grep defunct) and fd "
          "leaks from CgiHandler pipes after this run.")


# ---------------------------------------------------------------------------
# 12. Connection: close correctness (the bug you found)
# ---------------------------------------------------------------------------
def test_close_header_actually_closes():
    print("[close_header_actually_closes] verifying socket really closes "
          "after a Connection: close response...")

    # trigger a guaranteed error response with a bad request line
    req = b"GET\r\n\r\n"
    s = connect()
    s.send(req)
    resp = recv_all(s, timeout=3)
    has_close = b"connection: close" in resp.lower()
    print(f"  got Connection: close header: {has_close}")

    # now try to detect if the fd is still alive server-side by sending more
    try:
        s.send(b"GET / HTTP/1.1\r\nHost: x\r\n\r\n")
        time.sleep(0.5)
        extra = recv_all(s, timeout=2)
        if extra:
            print(f"  !! server responded AFTER Connection: close ({len(extra)}B) "
                  f"-> socket was not actually closed")
        else:
            # try to detect RST/EOF
            try:
                s.settimeout(1)
                d = s.recv(1)
                if d == b"":
                    print("  OK: recv() returned EOF, socket appears closed")
                else:
                    print("  !! unexpected data after close")
            except (ConnectionResetError, BrokenPipeError):
                print("  OK: connection reset/broken pipe, socket was closed")
            except socket.timeout:
                print("  !! no EOF/RST within timeout -> socket likely still "
                      "open server-side (matches the EPOLLIN/EPOLLOUT "
                      "starvation bug)")
    except (ConnectionResetError, BrokenPipeError):
        print("  OK: send failed with reset/broken pipe, socket was closed")
    finally:
        s.close()


# ---------------------------------------------------------------------------
# 13. Duplicate / conflicting headers (Host, Content-Type, Content-Length)
# ---------------------------------------------------------------------------
def test_duplicate_headers():
    print("[duplicate_headers] sending conflicting duplicate headers...")
    cases = {
        "dup_host_diff": b"GET / HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n",
        "dup_host_same": b"GET / HTTP/1.1\r\nHost: a\r\nHost: a\r\n\r\n",
        "dup_content_length_diff": (
            b"POST /upload HTTP/1.1\r\nHost: x\r\nContent-Length: 5\r\n"
            b"Content-Length: 10\r\nContent-Type: text/plain\r\n\r\nhello"
        ),
    }
    for name, payload in cases.items():
        try:
            s = connect()
            s.send(payload)
            resp = recv_all(s, timeout=3)
            status = resp.splitlines()[0] if resp else b"<no response>"
            print(f"  [{name}] -> {status}")
            s.close()
        except Exception as e:
            print(f"  [{name}] exception: {e}")


TESTS = {
    "connection_flood": test_connection_flood,
    "slowloris": test_slowloris,
    "malformed_lines": test_malformed_lines,
    "oversized_headers": test_oversized_headers,
    "chunked_abuse": test_chunked_abuse,
    "content_length_lies": test_content_length_lies,
    "body_size_boundary": test_body_size_boundary,
    "malformed_multipart": test_malformed_multipart,
    "pipelining": test_pipelining,
    "fd_leak": test_fd_leak,
    "cgi_concurrency": test_cgi_concurrency,
    "close_header_actually_closes": test_close_header_actually_closes,
    "duplicate_headers": test_duplicate_headers,
}


def main():
    global HOST, PORT
    parser = argparse.ArgumentParser(description="webserv stress test suite")
    parser.add_argument("--host", default=HOST)
    parser.add_argument("--port", type=int, default=PORT)
    parser.add_argument("--test", default="all", choices=list(TESTS.keys()) + ["all"])
    args = parser.parse_args()

    HOST, PORT = args.host, args.port
    print(f"Target: {HOST}:{PORT}\n")

    if args.test == "all":
        for name, fn in TESTS.items():
            print(f"\n===== {name} =====")
            try:
                fn()
            except KeyboardInterrupt:
                print("interrupted")
                sys.exit(1)
            except Exception as e:
                print(f"  test crashed with: {e}")
            time.sleep(0.5)
    else:
        TESTS[args.test]()


if __name__ == "__main__":
    main()
