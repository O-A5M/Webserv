#!/usr/bin/env python3

import os

print("Content-Type: text/html")
print()

print("""
<!DOCTYPE html>
<html>
<head>
    <title>CGI Test</title>
</head>
<body>
    <h1>CGI works!</h1>
    <p>Hello from Python CGI</p>
    <p>Method: {}</p>
    <p>Query: {}</p>
</body>
</html>
""".format(
    os.environ.get("REQUEST_METHOD"),
    os.environ.get("QUERY_STRING")
))
