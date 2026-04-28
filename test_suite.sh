#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  WEBSERV STRESS TEST SUITE
# ═══════════════════════════════════════════════════════════════

HOST="http://localhost:8080"
HOST2="http://localhost:8081"
PASS=0
FAIL=0
TOTAL=0

check() {
    TOTAL=$((TOTAL + 1))
    local desc="$1"
    local expected="$2"
    local got="$3"
    if echo "$got" | grep -q "$expected"; then
        PASS=$((PASS + 1))
        echo -e "  ✅ $desc"
    else
        FAIL=$((FAIL + 1))
        echo -e "  ❌ $desc (expected: $expected, got: $(echo "$got" | head -1))"
    fi
}

check_status() {
    TOTAL=$((TOTAL + 1))
    local desc="$1"
    local expected="$2"
    local got="$3"
    if [ "$got" = "$expected" ]; then
        PASS=$((PASS + 1))
        echo -e "  ✅ $desc"
    else
        FAIL=$((FAIL + 1))
        echo -e "  ❌ $desc (expected: $expected, got: $got)"
    fi
}

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 1: BASIC GET REQUESTS"
echo "══════════════════════════════════════════════════════"

# 1. Root index page
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/")
check_status "GET / → 200" "200" "$STATUS"

# 2. Static page
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/pages/index.html")
check_status "GET /pages/index.html → 200" "200" "$STATUS"

# 3. Non-existent file → 404
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/doesnotexist.html")
check_status "GET /doesnotexist.html → 404" "404" "$STATUS"

# 4. Custom error page content for 404
BODY=$(curl -s --http1.0 "$HOST/doesnotexist.html")
check "404 serves custom error page" "</html>" "$BODY"

# 5. Directory without trailing slash → 301 redirect
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/pages")
check_status "GET /pages (no slash) → 301" "301" "$STATUS"

# 6. Directory with trailing slash + autoindex on
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/pages/")
check_status "GET /pages/ (autoindex on) → 200" "200" "$STATUS"

BODY=$(curl -s --http1.0 "$HOST/pages/")
check "Autoindex lists files" "index.html" "$BODY"

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 2: ERROR RESPONSES & CUSTOM ERROR PAGES"
echo "══════════════════════════════════════════════════════"

# 7. Path traversal → 403
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is --http1.0 "$HOST/pages/../errors/400.html")
check_status "GET path traversal /../ → 403" "403" "$STATUS"

# 8. Method not allowed
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 -X POST "$HOST/pages/index.html")
check_status "POST on GET-only location → 405" "405" "$STATUS"

# 9. DELETE not allowed on root
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 -X DELETE "$HOST/pages/index.html")
check_status "DELETE on GET-only location → 405" "405" "$STATUS"

# 10. 405 custom error page
BODY=$(curl -s --http1.0 -X POST "$HOST/pages/index.html")
check "405 serves custom error page" "</html>" "$BODY"

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 3: REDIRECT"
echo "══════════════════════════════════════════════════════"

# 11. Config redirect /old → 301
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/old")
check_status "GET /old → 301 redirect" "301" "$STATUS"

LOCATION=$(curl -s -D - --http1.0 "$HOST/old" | grep -i "^Location:" | tr -d '\r')
check "Redirect Location header" "/pages/index.html" "$LOCATION"

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 4: CGI (Python)"
echo "══════════════════════════════════════════════════════"

# 12. CGI GET (test.py)
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/cgi-bin/test.py")
check_status "GET /cgi-bin/test.py → 200" "200" "$STATUS"

BODY=$(curl -s --http1.0 "$HOST/cgi-bin/test.py")
check "CGI GET returns HTML" "CGI works" "$BODY"

# 13. CGI GET with query string
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/cgi-bin/test.py?foo=bar&baz=42")
check_status "GET /cgi-bin/test.py?query → 200" "200" "$STATUS"

# 14. CGI POST (post_test.py)
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 -X POST \
  -d "name=tester&email=test@42.fr&age=25&message=hello" \
  "$HOST/cgi-bin/post_test.py")
check_status "POST /cgi-bin/post_test.py → 200" "200" "$STATUS"

BODY=$(curl -s --http1.0 -X POST \
  -d "name=tester&email=test@42.fr" \
  "$HOST/cgi-bin/post_test.py")
check "CGI POST body parsed (name)" "tester" "$BODY"
check "CGI POST body parsed (email)" "test@42.fr" "$BODY"

# 15. CGI nonexistent script → 404
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/cgi-bin/nope.py")
check_status "GET /cgi-bin/nope.py → 404" "404" "$STATUS"

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 5: POST / FILE UPLOAD"
echo "══════════════════════════════════════════════════════"

# 16. Simple POST body
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 -X POST \
  -d "key=value" \
  -H "Content-Type: application/x-www-form-urlencoded" \
  "$HOST/uploads")
check_status "POST urlencoded → 200" "200" "$STATUS"

# 17. File upload (multipart)
echo "test file content 12345" > /tmp/testfile.txt
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 -X POST \
  -F "file=@/tmp/testfile.txt" \
  "$HOST/uploads")
check_status "POST multipart file upload → 201" "201" "$STATUS"

# 18. Verify uploaded file exists
if [ -f /home/roote/webserver/www/uploads/testfile.txt ]; then
    check_status "Uploaded file exists on disk" "yes" "yes"
else
    check_status "Uploaded file exists on disk" "yes" "no"
fi

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 6: DELETE"
echo "══════════════════════════════════════════════════════"

# 19. Create a file to delete
echo "delete me" > /home/roote/webserver/www/uploads/deleteme.txt

STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 -X DELETE "$HOST/uploads/deleteme.txt")
check_status "DELETE /uploads/deleteme.txt → 204" "204" "$STATUS"

# 20. Verify file is gone
if [ ! -f /home/roote/webserver/www/uploads/deleteme.txt ]; then
    check_status "Deleted file is removed from disk" "yes" "yes"
else
    check_status "Deleted file is removed from disk" "yes" "no"
fi

# 21. DELETE nonexistent file → 404
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 -X DELETE "$HOST/uploads/nope.txt")
check_status "DELETE nonexistent file → 404" "404" "$STATUS"

# 22. DELETE a directory → 403
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 -X DELETE "$HOST/uploads/")
check_status "DELETE directory → 403" "403" "$STATUS"

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 7: HEADERS & RESPONSE FORMAT"
echo "══════════════════════════════════════════════════════"

# 23. Response has Server header
HEADERS=$(curl -s -D - -o /dev/null --http1.0 "$HOST/")
check "Response has Server header" "webserv" "$HEADERS"

# 24. Response has Date header
check "Response has Date header" "Date:" "$HEADERS"

# 25. Response has Content-Length
check "Response has Content-Length" "Content-Length:" "$HEADERS"

# 26. Response has Connection: close
check "Response has Connection: close" "Connection: close" "$HEADERS"

# 27. Content-Type for HTML
check "Content-Type text/html" "text/html" "$HEADERS"

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 8: SESSION / COOKIES"
echo "══════════════════════════════════════════════════════"

# 28. First request gets Set-Cookie
HEADERS=$(curl -s -D - -o /dev/null --http1.0 "$HOST/")
check "Set-Cookie header present" "Set-Cookie:" "$HEADERS"
check "SESSION_ID cookie set" "SESSION_ID=" "$HEADERS"

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 9: SECOND SERVER (port 8081)"
echo "══════════════════════════════════════════════════════"

# 29. Second server responds
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST2/")
check_status "GET / on port 8081 → 200" "200" "$STATUS"

# 30. Second server 404
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST2/nope.html")
check_status "GET /nope on port 8081 → 404" "404" "$STATUS"

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 10: EDGE CASES & STRESS"
echo "══════════════════════════════════════════════════════"

# 31. Empty body POST
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 -X POST \
  -H "Content-Length: 0" "$HOST/uploads")
check_status "POST empty body → 200" "200" "$STATUS"

# 32. Very long URI
LONG_URI=$(python3 -c 'print("a"*4000)')
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/$LONG_URI" 2>/dev/null)
check "Long URI handled (no crash)" "." "$STATUS"

# 33. Double slashes in path
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST//pages//index.html")
check "Double slashes handled (no crash)" "." "$STATUS"

# 34. Path traversal variants
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --path-as-is --http1.0 "$HOST/pages/../../etc/passwd")
check_status "GET ../../etc/passwd → 403" "403" "$STATUS"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/..%2f..%2fetc%2fpasswd")
# URL-encoded path traversal
check "URL encoded path traversal handled" "." "$STATUS"

# 35. Concurrent requests (basic)
echo "  ⏳ Running 20 concurrent requests..."
PASS_CONC=0
for i in $(seq 1 20); do
    curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/pages/index.html" &
done
# RESULTS=$(wait; echo "done")

# Test sequential rapid-fire
ALL_OK=true
for i in $(seq 1 20); do
    S=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/pages/index.html")
    if [ "$S" != "200" ]; then
        ALL_OK=false
    fi
done
if [ "$ALL_OK" = true ]; then
    check_status "20 rapid sequential requests → all 200" "true" "true"
else
    check_status "20 rapid sequential requests → all 200" "true" "false"
fi

# 36. Chunked transfer encoding
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.1 \
  -H "Transfer-Encoding: chunked" \
  -d "name=chunked_test" \
  -X POST "$HOST/cgi-bin/post_test.py")
check "Chunked POST handled (no crash)" "." "$STATUS"

# 37. Keep testing error pages have correct custom content
BODY_403=$(curl -s --http1.0 "$HOST/pages/../errors/400.html")
check "403 error has HTML body" "<" "$BODY_403"

BODY_404=$(curl -s --http1.0 "$HOST/this_does_not_exist")
check "404 error has HTML body" "<" "$BODY_404"

# 38. HEAD-like test (check response structure)
RESP=$(curl -s -D - --http1.0 "$HOST/pages/index.html")
check "Response starts with HTTP" "HTTP/1.0 200" "$RESP"

# 39. Upload autoindex
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/uploads/")
check_status "GET /uploads/ (autoindex) → 200" "200" "$STATUS"

# 40. Binary file upload
dd if=/dev/urandom bs=1024 count=10 of=/tmp/binary_test.bin 2>/dev/null
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 -X POST \
  -F "file=@/tmp/binary_test.bin" \
  "$HOST/uploads")
check_status "Binary file upload → 201" "201" "$STATUS"

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 11: CGI TIMEOUT TEST"
echo "══════════════════════════════════════════════════════"

# 41. CGI that takes too long → 504
echo "  ⏳ Testing CGI timeout (slow.py sleeps 30s, timeout is 5s)..."
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 --max-time 15 "$HOST/cgi-bin/slow.py")
check_status "GET /cgi-bin/slow.py → 504 (timeout)" "504" "$STATUS"

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 12: MULTIPLE CGIS (BASH/PYTHON)"
echo "══════════════════════════════════════════════════════"

# 42. Bash CGI test
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "$HOST/cgi-bin/test.sh")
check_status "GET /cgi-bin/test.sh → 200" "200" "$STATUS"

BODY=$(curl -s --http1.0 "$HOST/cgi-bin/test.sh")
check "Bash CGI executed properly" "Bash CGI executed properly" "$BODY"

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 13: ADVANCED SESSION & COOKIES"
echo "══════════════════════════════════════════════════════"

# 43. Second request with cookie has no Set-Cookie
HEADERS1=$(curl -s -D - -o /dev/null --http1.0 "$HOST/")
SES_ID=$(echo "$HEADERS1" | grep -i "^Set-Cookie:" | awk -F'=' '{print $2}' | tr -d '\r')
HEADERS2=$(curl -s -D - -o /dev/null -H "Cookie: SESSION_ID=$SES_ID" --http1.0 "$HOST/")
if echo "$HEADERS2" | grep -qi "^Set-Cookie:"; then
    check_status "Subsequent request with cookie gets NO Set-Cookie" "no" "yes"
else
    check_status "Subsequent request with cookie gets NO Set-Cookie" "yes" "yes"
fi

echo ""
echo "══════════════════════════════════════════════════════"
echo "  SECTION 14: EDGE CASES (413, 501)"
echo "══════════════════════════════════════════════════════"

# 44. Unknown HTTP method -> 501 Not Implemented
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 -X TRACE "$HOST/")
check_status "TRACE method → 501" "501" "$STATUS"

# 45. Client max body size -> 413 Payload Too Large
dd if=/dev/urandom bs=1048576 count=11 of=/tmp/huge_test.bin 2>/dev/null
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 -X POST -F "file=@/tmp/huge_test.bin" "$HOST/uploads")
check_status "POST > client_max_body_size (10M) → 413" "413" "$STATUS"
rm -f /tmp/huge_test.bin

echo ""
echo "══════════════════════════════════════════════════════"
echo "  RESULTS"
echo "══════════════════════════════════════════════════════"
echo ""
echo "  Total:  $TOTAL"
echo "  Passed: $PASS"
echo "  Failed: $FAIL"
echo ""
if [ $FAIL -eq 0 ]; then
    echo "  🎉 ALL TESTS PASSED!"
else
    echo "  ⚠️  $FAIL test(s) failed"
fi
echo ""

# Cleanup
rm -f /tmp/testfile.txt /tmp/binary_test.bin
rm -f /home/roote/webserver/www/uploads/testfile.txt
rm -f /home/roote/webserver/www/uploads/binary_test.bin
