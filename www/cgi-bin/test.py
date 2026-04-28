#!/usr/bin/env python3
import os

# 1. Print the necessary HTTP Headers
print("Content-Type: text/html\r")
print("\r")

# 2. Path to your HTML file relative to this script
file_path = os.path.join(os.path.dirname(__file__), "../pages/cgi_success.html")

try:
    # 3. Read and output the file content
    with open(file_path, "r") as file:
        print(file.read())
except IOError:
    # Fallback if the file isn't found
    print("<html><body><h1>CGI Success page.</h1></body></html>")