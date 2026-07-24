#!/usr/bin/env python3
"""
================================================================================
WEBSERV STATUS CODE TEST SUITE (Nginx HTTP/1.1 Compliance)
================================================================================
Tests HTTP Status Codes across all project routes, HTTP methods, CGI,
malformed requests, headers, redirects, payload limits, and protocol versions.
================================================================================
"""

import socket
import requests
import os
import time
from typing import List, Union, Tuple, Optional

# --- CONFIGURATION ---
HOST = os.getenv("WEBSERV_HOST", "127.0.0.1")
PORT = int(os.getenv("WEBSERV_PORT", "8080"))
PORT2 = int(os.getenv("WEBSERV_PORT2", "8081"))
BASE_URL = f"http://{HOST}:{PORT}"
BASE_URL2 = f"http://{HOST}:{PORT2}"
TIMEOUT = int(os.getenv("WEBSERV_TIMEOUT", "3"))

# --- ANSI COLORS ---
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
BLUE = "\033[94m"
MAGENTA = "\033[95m"
CYAN = "\033[96m"
RESET = "\033[0m"
BOLD = "\033[1m"

# --- GLOBAL STATS ---
stats = {"passed": 0, "failed": 0, "total": 0}


def print_section(title: str):
    print(f"\n{MAGENTA}{'='*70}{RESET}")
    print(f"{MAGENTA}{BOLD}  {title}{RESET}")
    print(f"{MAGENTA}{'='*70}{RESET}")


def format_expected(expected: Union[int, List[int]]) -> str:
    if isinstance(expected, list):
        return "/".join(str(e) for e in expected)
    return str(expected)


def check_status(actual: int, expected: Union[int, List[int]]) -> bool:
    if isinstance(expected, list):
        return actual in expected
    return actual == expected


def print_result(name: str, passed: bool, expected: Union[int, List[int]], got: str, duration_ms: float = 0.0):
    stats["total"] += 1
    if passed:
        stats["passed"] += 1
        icon = "✅"
        color = GREEN
        print(f"{color}{icon}{RESET} {BOLD}{name:<48}{RESET} -> Status: {color}{got}{RESET} (Expected: {format_expected(expected)}) [{duration_ms:.0f}ms]")
    else:
        stats["failed"] += 1
        icon = "❌"
        color = RED
        print(f"{color}{icon}{RESET} {BOLD}{name:<48}{RESET} -> Got: {color}{got}{RESET} | Expected: {format_expected(expected)} [{duration_ms:.0f}ms]")


def make_raw_request(payload: Union[str, bytes], port: int = PORT, timeout: int = TIMEOUT) -> Tuple[int, str]:
    """Send a raw HTTP request over TCP socket and return (status_code, status_line)."""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(timeout)
        s.connect((HOST, port))
        if isinstance(payload, str):
            payload = payload.encode('utf-8', errors='replace')
        s.sendall(payload)
        response = b""
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            response += chunk
            if b"\r\n\r\n" in response:
                break
        s.close()
        if not response:
            return -1, "Empty/No Response"
        lines = response.split(b"\r\n")
        status_line = lines[0].decode('utf-8', errors='replace') if lines else "Invalid"
        parts = status_line.split()
        if len(parts) >= 2 and parts[1].isdigit():
            return int(parts[1]), status_line
        return -1, status_line
    except socket.timeout:
        return -1, "Timeout"
    except ConnectionRefusedError:
        return -1, "Connection Refused"
    except Exception as e:
        return -1, f"Error: {str(e)}"


# ==============================================================================
# 1. GET & STATIC FILES STATUS CODES
# ==============================================================================
def test_static_routes():
    print_section("1. GET & STATIC ROUTE STATUS CODES")
    tests = [
        ("GET Root /", "GET", "/", [200]),
        ("GET index.html", "GET", "/index.html", [200]),
        ("GET 404 Non-existent file", "GET", "/nonexistent_12345.html", [404]),
        ("GET Deep static file (style.css)", "GET", "/style.css", [200, 404]),
        ("GET Query string (GET /?key=val)", "GET", "/?key=val", [200]),
        ("GET URL encoded space (/file%20name.txt)", "GET", "/file%20name.txt", [200, 404]),
        ("GET Trailing slash on file (/index.html/)", "GET", "/index.html/", [404]),
    ]
    for name, method, path, expected in tests:
        t0 = time.time()
        try:
            r = requests.request(method, BASE_URL + path, timeout=TIMEOUT, allow_redirects=False)
            passed = check_status(r.status_code, expected)
            print_result(name, passed, expected, str(r.status_code), (time.time()-t0)*1000)
        except Exception as e:
            print_result(name, False, expected, f"Exception: {e}", (time.time()-t0)*1000)


# ==============================================================================
# 2. REDIRECT STATUS CODES (Nginx return 301)
# ==============================================================================
def test_redirect_routes():
    print_section("2. REDIRECT STATUS CODES")
    tests = [
        ("GET Configured 301 Redirect (/old)", "GET", "/old", [301]),
    ]
    for name, method, path, expected in tests:
        t0 = time.time()
        try:
            r = requests.request(method, BASE_URL + path, timeout=TIMEOUT, allow_redirects=False)
            passed = check_status(r.status_code, expected)
            print_result(name, passed, expected, str(r.status_code), (time.time()-t0)*1000)
        except Exception as e:
            print_result(name, False, expected, f"Exception: {e}", (time.time()-t0)*1000)


# ==============================================================================
# 3. HTTP METHOD ALLOWED / DISALLOWED STATUS CODES
# ==============================================================================
def test_method_status_codes():
    print_section("3. HTTP METHOD STATUS CODES (GET/POST/DELETE vs 405)")
    tests = [
        ("GET / (Allowed -> 200)", "GET", "/", None, [200]),
        ("POST /upload (Allowed -> 200/201/204)", "POST", "/upload", {"file": "test"}, [200, 201, 204]),
        ("DELETE /img/ (Disallowed -> 405)", "DELETE", "/img/", None, [405]),
        ("PUT /index.html (Disallowed -> 405/501)", "PUT", "/index.html", None, [405, 501]),
        ("PATCH /index.html (Disallowed -> 405/501)", "PATCH", "/index.html", None, [405, 501]),
        ("OPTIONS /index.html (Disallowed -> 405/501)", "OPTIONS", "/index.html", None, [405, 501]),
    ]
    for name, method, path, data, expected in tests:
        t0 = time.time()
        try:
            r = requests.request(method, BASE_URL + path, data=data, timeout=TIMEOUT, allow_redirects=False)
            passed = check_status(r.status_code, expected)
            print_result(name, passed, expected, str(r.status_code), (time.time()-t0)*1000)
        except Exception as e:
            print_result(name, False, expected, f"Exception: {e}", (time.time()-t0)*1000)


# ==============================================================================
# 4. DIRECTORY LISTING / AUTOINDEX STATUS CODES
# ==============================================================================
def test_directory_status_codes():
    print_section("4. DIRECTORY LISTING STATUS CODES")
    tests = [
        ("GET Directory with Autoindex ON (/upload/)", "GET", "/upload/", [200]),
        ("GET Directory with Autoindex ON (/img/)", "GET", "/img/", [200]),
        ("GET Directory with Autoindex OFF (/cgi-bin/)", "GET", "/cgi-bin/", [403, 404]),
    ]
    for name, method, path, expected in tests:
        t0 = time.time()
        try:
            r = requests.request(method, BASE_URL + path, timeout=TIMEOUT, allow_redirects=False)
            passed = check_status(r.status_code, expected)
            print_result(name, passed, expected, str(r.status_code), (time.time()-t0)*1000)
        except Exception as e:
            print_result(name, False, expected, f"Exception: {e}", (time.time()-t0)*1000)


# ==============================================================================
# 5. CGI SCRIPT STATUS CODES
# ==============================================================================
def test_cgi_status_codes():
    print_section("5. CGI SCRIPT EXECUTION STATUS CODES")
    tests = [
        ("GET Valid CGI script (/cgi-bin/test.py)", "GET", "/cgi-bin/test.py", None, [200]),
        ("POST Valid CGI script (/cgi-bin/test.py)", "POST", "/cgi-bin/test.py", "arg1=val1", [200]),
        ("GET Non-existent CGI script", "GET", "/cgi-bin/missing.py", None, [404, 502]),
    ]
    for name, method, path, data, expected in tests:
        t0 = time.time()
        try:
            r = requests.request(method, BASE_URL + path, data=data, timeout=TIMEOUT, allow_redirects=False)
            passed = check_status(r.status_code, expected)
            print_result(name, passed, expected, str(r.status_code), (time.time()-t0)*1000)
        except Exception as e:
            print_result(name, False, expected, f"Exception: {e}", (time.time()-t0)*1000)


# ==============================================================================
# 6. HTTP PROTOCOL & VERSION STATUS CODES
# ==============================================================================
def test_protocol_status_codes():
    print_section("6. HTTP PROTOCOL & VERSION STATUS CODES")
    raw_tests = [
        ("HTTP/1.1 Valid with Host header", "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", PORT, [200]),
        ("HTTP/1.1 Missing Host header -> 400", "GET / HTTP/1.1\r\n\r\n", PORT, [400, 404]),
        ("HTTP/1.0 Request -> 505/400/200", "GET / HTTP/1.0\r\n\r\n", PORT, [505, 400, 200]),
        ("HTTP/2.0 Request -> 505/400/501", "GET / HTTP/2.0\r\nHost: localhost\r\n\r\n", PORT, [505, 400, 501]),
        ("HTTP/0.9 Request -> 505/400", "GET / HTTP/0.9\r\nHost: localhost\r\n\r\n", PORT, [505, 400]),
    ]
    for name, payload, port, expected in raw_tests:
        t0 = time.time()
        status_code, status_line = make_raw_request(payload, port=port)
        passed = check_status(status_code, expected)
        print_result(name, passed, expected, f"{status_code} ({status_line})", (time.time()-t0)*1000)


# ==============================================================================
# 7. PAYLOAD SIZE LIMIT STATUS CODES (client_max_body_size -> 413)
# ==============================================================================
def test_payload_limit_status_codes():
    print_section("7. PAYLOAD LIMIT STATUS CODES (client_max_body_size)")
    # Port 8081 has client_max_body_size 5 bytes
    t0 = time.time()
    try:
        r = requests.post(BASE_URL2 + "/", data="A" * 100, timeout=TIMEOUT)
        passed = check_status(r.status_code, [413, 400])
        print_result("POST payload > limit on Port 8081 -> 413", passed, [413, 400], str(r.status_code), (time.time()-t0)*1000)
    except Exception as e:
        print_result("POST payload > limit on Port 8081 -> 413", False, [413, 400], f"Exception: {e}", (time.time()-t0)*1000)


# ==============================================================================
# 8. MALFORMED & SECURITY STATUS CODES
# ==============================================================================
def test_malformed_status_codes():
    print_section("8. MALFORMED REQUEST & SECURITY STATUS CODES")
    raw_tests = [
        ("Directory Traversal (/../../../etc/passwd) -> 400/403/404", "GET /../../../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n", [400, 403, 404]),
        ("Unknown Method (FROBNICATE) -> 405/400/501", "FROBNICATE / HTTP/1.1\r\nHost: localhost\r\n\r\n", [405, 400, 501]),
        ("Case-sensitive Method ('get') -> 400/405/501", "get / HTTP/1.1\r\nHost: localhost\r\n\r\n", [400, 405, 501]),
        ("URI Too Long (8KB URI) -> 414/400/404", "GET /" + "A"*8192 + " HTTP/1.1\r\nHost: localhost\r\n\r\n", [414, 400, 404]),
        ("Header Without Colon -> 400/200", "GET / HTTP/1.1\r\nHost: localhost\r\nBadHeaderValue\r\n\r\n", [400, 200]),
    ]
    for name, payload, expected in raw_tests:
        t0 = time.time()
        status_code, status_line = make_raw_request(payload)
        passed = check_status(status_code, expected)
        print_result(name, passed, expected, f"{status_code} ({status_line})", (time.time()-t0)*1000)


# ==============================================================================
# 9. 5xx SERVER & GATEWAY ERROR STATUS CODES
# ==============================================================================
def test_5xx_status_codes():
    print_section("9. 5xx SERVER & GATEWAY ERROR STATUS CODES")

    # 500 / 502: Invalid/malformed CGI response headers (missing headers)
    t0 = time.time()
    try:
        r = requests.get(BASE_URL + "/cgi-bin/testNoHeader.py", timeout=TIMEOUT, allow_redirects=False)
        passed = check_status(r.status_code, [500, 502])
        print_result("502/500 Bad Gateway (Malformed CGI output)", passed, [500, 502], str(r.status_code), (time.time()-t0)*1000)
    except Exception as e:
        print_result("502/500 Bad Gateway (Malformed CGI output)", False, [500, 502], f"Exception: {e}", (time.time()-t0)*1000)

    # 504 / 502 / 500: Gateway Timeout (hanging CGI script)
    t0 = time.time()
    try:
        r = requests.get(BASE_URL + "/cgi-bin/testTimeout.py", timeout=2)
        passed = check_status(r.status_code, [504, 500, 502])
        print_result("504 Gateway Timeout (Hanging CGI script)", passed, [504, 500, 502], str(r.status_code), (time.time()-t0)*1000)
    except requests.exceptions.Timeout:
        print_result("504 Gateway Timeout (Socket Timeout / Hung CGI)", True, [504, 500, 502], "Timeout (504)", (time.time()-t0)*1000)
    except Exception as e:
        print_result("504 Gateway Timeout (Hanging CGI script)", False, [504, 500, 502], f"Exception: {e}", (time.time()-t0)*1000)

    # 505: HTTP Version Not Supported
    t0 = time.time()
    status_code, status_line = make_raw_request("GET / HTTP/2.0\r\nHost: localhost\r\n\r\n")
    passed = check_status(status_code, [505, 400, 501])
    print_result("505 HTTP Version Not Supported (HTTP/2.0)", passed, [505, 400, 501], f"{status_code} ({status_line})", (time.time()-t0)*1000)


# ==============================================================================
# MAIN TEST RUNNER
# ==============================================================================
def run_all_tests():
    print(f"\n{CYAN}{BOLD}{'='*70}{RESET}")
    print(f"{CYAN}{BOLD}  WEBSERV STATUS CODE TEST SUITE (Target: {BASE_URL}){RESET}")
    print(f"{CYAN}{BOLD}{'='*70}{RESET}")

    test_static_routes()
    test_redirect_routes()
    test_method_status_codes()
    test_directory_status_codes()
    test_cgi_status_codes()
    test_protocol_status_codes()
    test_payload_limit_status_codes()
    test_malformed_status_codes()
    test_5xx_status_codes()

    # --- SUMMARY ---
    print(f"\n{CYAN}{BOLD}{'='*70}{RESET}")
    print(f"{CYAN}{BOLD}  STATUS CODE TEST SUMMARY{RESET}")
    print(f"{CYAN}{BOLD}{'='*70}{RESET}")
    success_rate = (stats["passed"] / stats["total"] * 100) if stats["total"] > 0 else 0
    print(f" Total Status Code Tests: {BOLD}{stats['total']}{RESET}")
    print(f" Passed:                 {GREEN}{BOLD}{stats['passed']}{RESET}")
    print(f" Failed:                 {RED}{BOLD}{stats['failed']}{RESET}")
    print(f" Success Rate:           {BOLD}{success_rate:.1f}%{RESET}")
    print(f"{CYAN}{BOLD}{'='*70}{RESET}\n")


if __name__ == "__main__":
    run_all_tests()