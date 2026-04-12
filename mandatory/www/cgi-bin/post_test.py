#!/usr/bin/env python3
import sys
import os

content_length = int(os.environ.get('CONTENT_LENGTH', 0))
body = sys.stdin.read(content_length) if content_length > 0 else ""

# parse key=value pairs
params = {}
if body:
    for pair in body.split('&'):
        if '=' in pair:
            key, value = pair.split('=', 1)
            params[key] = value.replace('+', ' ')

name    = params.get('name', 'stranger')
email   = params.get('email', '')
age     = params.get('age', '')
message = params.get('message', '')

print("Content-Type: text/html\r")
print("\r")
print("""<!DOCTYPE html>
<html><head><meta charset="UTF-8"><title>CGI Response</title></head>
<body>
<h2>CGI Response</h2>
<table>""")
print(f"<tr><td><b>Name</b></td><td>{name}</td></tr>")
if email:
    print(f"<tr><td><b>Email</b></td><td>{email}</td></tr>")
if age:
    print(f"<tr><td><b>Age</b></td><td>{age}</td></tr>")
if message:
    print(f"<tr><td><b>Message</b></td><td>{message}</td></tr>")
print("</table></body></html>")