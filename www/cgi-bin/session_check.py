import os

print("Content-Type: text/html\r\n\r\n", end="")

print("<!DOCTYPE html>")
print("<html>")
print("<head>")
print("    <title>Session Checker</title>")
print("    <style>")
print("      body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }")
print("      h1 { font-size: 3em; }")
print("    </style>")
print("</head>")
print("<body>")

cookie_header = os.environ.get("HTTP_COOKIE", "")

if "SESSION_ID" in cookie_header:
    print("    <h1 style='color: green;'>HELLO SESSION IS ALIVE !!</h1>")
else:
    print("    <h1 style='color: red;'>NO SESSION YET</h1>")
print("</body>")
print("</html>")
