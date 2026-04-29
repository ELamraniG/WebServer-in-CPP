#!/bin/bash
printf "Content-Type: text/html\r\n\r\n"
echo "<!DOCTYPE html>"
echo "<html><body>"
echo "<h1>Bash CGI Test</h1>"
echo "<p>Method: $REQUEST_METHOD</p>"
echo "<p>Bash CGI executed properly</p>"
echo "</body></html>"
