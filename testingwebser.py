import socket
import threading
import time
import sys

# Configuration
HOST = '127.0.0.1'
PORT = 8080
NUM_TESTS = 200
CONCURRENCY = 20

def create_request(method, path, headers=None, body=""):
    req = f"{method} {path} HTTP/1.1\r\n"
    req += f"Host: {HOST}:{PORT}\r\n"
    if headers:
        for k, v in headers.items():
            req += f"{k}: {v}\r\n"
    if body:
        req += f"Content-Length: {len(body)}\r\n"
    req += "\r\n"
    req += body
    return req.encode('utf-8')

def send_request(test_id, req_bytes, results):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5.0)
            s.connect((HOST, PORT))
            s.sendall(req_bytes)
            
            # Read response
            response = b""
            while True:
                chunk = s.recv(4096)
                if not chunk:
                    break
                response += chunk
                # Simple check for end of headers if no body expected, 
                # or just read a bit to confirm 200 OK
                if b"\r\n\r\n" in response:
                    break
                    
        status_line = response.split(b"\r\n")[0].decode('utf-8', errors='ignore')
        if "200" in status_line:
            results[test_id] = "PASS"
        else:
            results[test_id] = f"FAIL: {status_line}"
            
    except Exception as e:
        results[test_id] = f"ERROR: {e}"

def run_tests():
    print(f"Starting {NUM_TESTS} tests against {HOST}:{PORT}...")
    
    import random
    
    results = {}
    threads = []
    
    start_time = time.time()
    
    for i in range(NUM_TESTS):
        # We can implement a simple concurrency limit
        while threading.active_count() > CONCURRENCY:
            time.sleep(0.01)
            
        if random.random() > 0.5:
            req = create_request("GET", "/")
        else:
            req = create_request("POST", "/upload", headers={"Content-Type": "text/plain"}, body="stress test payload")
            
        t = threading.Thread(target=send_request, args=(i, req, results))
        threads.append(t)
        t.start()
        
    for t in threads:
        t.join()
        
    end_time = time.time()
    
    # Analyze results
    passed = sum(1 for res in results.values() if res == "PASS")
    failed = len(results) - passed
    
    print("\n--- Test Results ---")
    print(f"Total Tests: {NUM_TESTS}")
    print(f"Passed: {passed}")
    print(f"Failed/Errors: {failed}")
    print(f"Time taken: {end_time - start_time:.2f} seconds")
    
    if failed > 0:
        print("\nSample of failures:")
        failures = {k: v for k, v in results.items() if v != "PASS"}
        for k, v in list(failures.items())[:10]:
            print(f"Test {k}: {v}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        PORT = int(sys.argv[1])
    run_tests()
