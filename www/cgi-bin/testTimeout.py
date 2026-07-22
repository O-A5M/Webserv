#!/usr/bin/env python3
"""
Timeout test script for webserv.

Tests four scenarios:
  1. idle              - connect, send nothing, wait past the idle timeout,
                          expect the server to close the connection (recv
                          returns b"").
  2. slowloris         - send request line + headers one byte at a time,
                          slower than the timeout, expect a 408 (or a close)
                          before completing.
  3. keepalive-trickle - the counterpart to slowloris: send 1 byte at a time
                          too, but with gaps SHORTER than the timeout, for a
                          total duration LONGER than the timeout. Proves that
                          Touch() is actually resetting the idle clock on
                          partial reads, not just on full requests. The
                          connection should survive the whole run.
  4. cgi-hang          - send a complete request to a CGI script that sleeps
                          longer than the CGI timeout, expect a 504 (or a
                          close) instead of hanging forever.

Usage:
    python3 test_timeouts.py idle              --host 127.0.0.1 --port 8080 --wait 65
    python3 test_timeouts.py slowloris         --host 127.0.0.1 --port 8080 --wait 65
    python3 test_timeouts.py keepalive-trickle --host 127.0.0.1 --port 8080 \\
        --timeout-hint 60 --duration 90
    python3 test_timeouts.py cgi-hang          --host 127.0.0.1 --port 8080 \\
        --path /cgi-bin/sleep.py --wait 35

Adjust --wait to be a few seconds longer than whatever timeout you configured
in ClientHandler / CgiHandler (SetTimeout(60), SetTimeout(30), etc).
"""

import argparse
import socket
import sys
import time


def connect(host, port, connect_timeout=5):
    s = socket.create_connection((host, port), timeout=connect_timeout)
    return s


def recv_all_nonblocking(sock, read_timeout=5):
    """Try to read whatever the server sends back within read_timeout seconds."""
    sock.settimeout(read_timeout)
    chunks = []
    try:
        while True:
            data = sock.recv(4096)
            if not data:
                # peer closed the connection
                return b"".join(chunks), True
            chunks.append(data)
    except socket.timeout:
        return b"".join(chunks), False


def test_idle(host, port, wait):
    print(f"[idle] connecting to {host}:{port} and sending nothing for {wait}s...")
    s = connect(host, port)
    start = time.time()

    # Don't send anything. Just wait, then try to read.
    body, closed = recv_all_nonblocking(s, read_timeout=wait)
    elapsed = time.time() - start

    if closed:
        print(f"[idle] PASS: server closed the idle connection after ~{elapsed:.1f}s")
    else:
        print(f"[idle] FAIL: connection still open after {elapsed:.1f}s, no data/close received")
        print(f"        (received {len(body)} bytes: {body[:200]!r})")
    s.close()


def test_slowloris(host, port, wait):
    """
    Send a valid request line, then trickle headers in 1 byte at a time,
    slower than the server's per-connection idle/read timeout, and never
    send the final blank line. The server should give up before we finish.
    """
    print(f"[slowloris] connecting to {host}:{port}, trickling headers over ~{wait}s...")
    s = connect(host, port)
    s.settimeout(2)

    request_line = b"GET / HTTP/1.1\r\n"
    header_line = b"X-Slow-Header: filler-value-to-keep-things-going\r\n"
    # We'll keep sending small header fragments, pausing between each,
    # but never send the terminating "\r\n\r\n".
    try:
        s.sendall(request_line)
    except (BrokenPipeError, ConnectionResetError, socket.timeout):
        print("[slowloris] connection dropped before we even sent the request line (unexpected)")
        s.close()
        return

    start = time.time()
    got_response = False
    response_data = b""

    while time.time() - start < wait:
        try:
            # trickle a few bytes at a time
            for b in header_line:
                s.sendall(bytes([b]))
                time.sleep(0.5)
                # opportunistically peek for a response (e.g. 408) without blocking long
                s.settimeout(0.1)
                try:
                    chunk = s.recv(4096)
                    if chunk:
                        response_data += chunk
                        got_response = True
                    elif chunk == b"":
                        print(f"[slowloris] PASS: server closed connection after "
                              f"~{time.time()-start:.1f}s of slow headers")
                        s.close()
                        return
                except socket.timeout:
                    pass
                s.settimeout(2)
        except (BrokenPipeError, ConnectionResetError):
            print(f"[slowloris] PASS: server reset/closed connection after "
                  f"~{time.time()-start:.1f}s of slow headers")
            s.close()
            return

    elapsed = time.time() - start
    if got_response:
        print(f"[slowloris] PASS: server responded (likely 408) after ~{elapsed:.1f}s")
        print(f"        response: {response_data[:200]!r}")
    else:
        print(f"[slowloris] FAIL: server neither responded nor closed after {elapsed:.1f}s "
              f"of trickled headers -- timeout not enforced")
    s.close()


def test_keepalive_trickle(host, port, timeout_hint, total_duration):
    """
    Proves that Touch() actually resets the idle timer on partial activity.

    Sends 1 byte every (timeout_hint / 2) seconds for a total of
    total_duration seconds (which should be LONGER than timeout_hint).
    Each individual gap is well under the server's configured timeout, so if
    Touch() is wired correctly on every OnRead(), the connection must survive
    the whole run even though total_duration > timeout_hint.

    If the connection dies before total_duration elapses, Touch() is either
    missing, misplaced, or not resetting the timer as expected.
    """
    gap = max(timeout_hint / 2.0, 1.0)
    print(f"[keepalive-trickle] connecting to {host}:{port}")
    print(f"        sending 1 byte every {gap:.1f}s for {total_duration:.0f}s total "
          f"(timeout hint: {timeout_hint:.0f}s)")
    print(f"        expectation: connection should stay OPEN the whole time, "
          f"since no single gap exceeds the timeout")

    s = connect(host, port)
    request_line = b"GET / HTTP/1.1\r\n"
    header_bytes = b"X-Keepalive-Trickle: still-here\r\n"

    try:
        s.sendall(request_line)
    except (BrokenPipeError, ConnectionResetError, socket.timeout):
        print("[keepalive-trickle] FAIL: connection dropped before request line was even sent")
        s.close()
        return

    start = time.time()
    idx = 0
    while time.time() - start < total_duration:
        b = header_bytes[idx % len(header_bytes)]
        idx += 1
        try:
            s.sendall(bytes([b]))
        except (BrokenPipeError, ConnectionResetError):
            elapsed = time.time() - start
            print(f"[keepalive-trickle] FAIL: connection died after ~{elapsed:.1f}s "
                  f"(before the {total_duration:.0f}s target) -- Touch() likely not "
                  f"resetting the timer correctly")
            s.close()
            return

        # opportunistic non-blocking peek for an unexpected early close/response
        s.settimeout(0.1)
        try:
            chunk = s.recv(4096)
            if chunk == b"":
                elapsed = time.time() - start
                print(f"[keepalive-trickle] FAIL: server closed after ~{elapsed:.1f}s "
                      f"despite steady trickling -- Touch() likely not firing")
                s.close()
                return
            elif chunk:
                elapsed = time.time() - start
                print(f"[keepalive-trickle] NOTE: got unexpected data at ~{elapsed:.1f}s: {chunk[:200]!r}")
        except socket.timeout:
            pass

        time.sleep(gap)

    elapsed = time.time() - start
    print(f"[keepalive-trickle] PASS: connection survived {elapsed:.1f}s of steady trickling "
          f"(longer than the {timeout_hint:.0f}s timeout hint) -- Touch() is resetting the timer correctly")
    s.close()


def test_cgi_hang(host, port, wait, path):
    """
    Sends a complete, valid request to a CGI script assumed to sleep/hang
    longer than the CGI timeout. Expects a 504 (or connection close) instead
    of hanging indefinitely.
    """
    print(f"[cgi-hang] connecting to {host}:{port}, requesting {path} "
          f"(expected to hang), waiting up to {wait}s...")
    s = connect(host, port)

    request = (
        f"GET {path} HTTP/1.1\r\n"
        f"Host: {host}\r\n"
        f"Connection: close\r\n"
        f"\r\n"
    ).encode()

    start = time.time()
    s.sendall(request)

    body, closed = recv_all_nonblocking(s, read_timeout=wait)
    elapsed = time.time() - start

    if body:
        status_line = body.split(b"\r\n", 1)[0]
        print(f"[cgi-hang] response after ~{elapsed:.1f}s: {status_line!r}")
        if b"504" in status_line:
            print("[cgi-hang] PASS: got 504 Gateway Timeout as expected")
        else:
            print("[cgi-hang] NOTE: got a response, but not a 504 -- check status code above")
    elif closed:
        print(f"[cgi-hang] PARTIAL PASS: server closed the connection after ~{elapsed:.1f}s "
              f"(no 504 body, but it did give up on the hung CGI)")
    else:
        print(f"[cgi-hang] FAIL: still hanging after {elapsed:.1f}s -- CGI timeout not enforced")

    s.close()


def main():
    parser = argparse.ArgumentParser(description="Test webserv connection/CGI timeouts")
    parser.add_argument("mode", choices=["idle", "slowloris", "keepalive-trickle", "cgi-hang"])
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--wait", type=float, default=65,
                        help="how long to wait for the timeout to trigger (seconds) "
                             "[idle / slowloris / cgi-hang]")
    parser.add_argument("--timeout-hint", type=float, default=60,
                        help="the client idle timeout you configured server-side "
                             "(SetTimeout value) [keepalive-trickle]")
    parser.add_argument("--duration", type=float, default=90,
                        help="total test duration, should exceed --timeout-hint "
                             "[keepalive-trickle]")
    parser.add_argument("--path", default="/cgi-bin/sleep.py",
                        help="CGI path to request for cgi-hang mode")
    args = parser.parse_args()

    if args.mode == "idle":
        test_idle(args.host, args.port, args.wait)
    elif args.mode == "slowloris":
        test_slowloris(args.host, args.port, args.wait)
    elif args.mode == "keepalive-trickle":
        test_keepalive_trickle(args.host, args.port, args.timeout_hint, args.duration)
    elif args.mode == "cgi-hang":
        test_cgi_hang(args.host, args.port, args.wait, args.path)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(1)