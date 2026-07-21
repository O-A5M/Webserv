#!/usr/bin/env python3
# Point a location's cgi_extension/.py at this and route to /cgi-bin/sleep.py
# It sleeps well past a 30s CGI timeout so you can confirm the server kills
# it and returns a 504 instead of hanging forever.
import sys
import time

time.sleep(120)

sys.stdout.write("Content-Type: text/plain\r\n\r\n")
sys.stdout.write("this should never actually be seen if the timeout works\n")