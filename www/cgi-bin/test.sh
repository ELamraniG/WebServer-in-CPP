#!/bin/bash

# 1. Print HTTP Headers for HTTP/1.0 (Required for webserv)
printf "Content-Type: text/html\r\n\r\n"

# 2. Get the directory where THIS script is located
# Equivalent to os.path.dirname(__file__) in Python
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 3. Define the path to the HTML file relative to the script
# This points to one directory up, then into the 'pages' folder
FILE_PATH="$SCRIPT_DIR/../pages/cgi_process.html"

# 4. Check if the file exists and output it
if [ -f "$FILE_PATH" ]; then
    # Inject Bash variables into the HTML template using sed
    sed -e "s/{{PID}}/$$/g" \
        -e "s/{{UPTIME}}/$(uptime -p)/g" \
        -e "s/{{METHOD}}/$REQUEST_METHOD/g" \
        "$FILE_PATH"
else
    # Fallback Error Page (Styled)
    echo "<html><body style='background:#04050f;color:#f43f5e;font-family:sans-serif;text-align:center;padding:50px;'>"
    echo "<h1>404 Template Not Found</h1>"
    echo "<p>Looked in: $FILE_PATH</p>"
    echo "</body></html>"
fi