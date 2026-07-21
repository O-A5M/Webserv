#!/usr/bin/env python3

import os
import sys

print("Content-Type: text/html")
print()

print("""<!DOCTYPE html>
<html>
<head>
    <title>Python CGI Test</title>
</head>
<body>
    <h1>Python CGI Works!</h1>

    <h2>Python Info</h2>
    <pre>Python version: {}</pre>

    <h2>CGI Environment</h2>
    <table border="1" cellpadding="5">
        <tr><th>Variable</th><th>Value</th></tr>
""".format(sys.version))

for key in sorted(os.environ):
    print(f"<tr><td>{key}</td><td>{os.environ[key]}</td></tr>")

print("""
    </table>
</body>
</html>
""")
