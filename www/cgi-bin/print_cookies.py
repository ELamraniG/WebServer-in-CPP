#!/usr/bin/env python3
import os
import sys

sys.stdout.write("Content-Type: text/html\r\n\r\n")

print("<html><body>")
print("<h1>Cookies Received:</h1>")

cookies = os.environ.get("HTTP_COOKIE", "No cookies found")

if cookies != "No cookies found":
    print("<ul>")
    cookiees = cookies.split(";")
    for cookie in cookiees:
        print("<li>" + cookie.strip() + "</li>")
    print("</ul>")

print("</body></html>")
