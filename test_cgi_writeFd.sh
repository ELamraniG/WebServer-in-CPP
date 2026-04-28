#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  TEST: CGI write-fd cleanup bugs
#  Bug #3: write-fd not cleaned on timeout (handleCGITimeout)
#  Bug #4: write-fd not cleaned on completion (handleCGIRead)
# ═══════════════════════════════════════════════════════════════

HOST="http://localhost:8080"
PASS=0
FAIL=0

check_status() {
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
echo "═══════════════════════════════════════════════════════"
echo "  TEST 1: CGI Timeout with POST body (Bug #3)"
echo "  Sends POST to slow.py → triggers timeout with write-fd still open"
echo "═══════════════════════════════════════════════════════"

# Generate a moderate POST body
BODY=$(python3 -c 'print("x" * 1000)')

echo "  ⏳ Sending POST to slow.py (will timeout in ~2s)..."
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 --max-time 10 \
  -X POST -d "$BODY" "$HOST/cgi-bin/slow.py")
check_status "POST /cgi-bin/slow.py → 504 (timeout)" "504" "$STATUS"

# After the timeout, the server should still be healthy.
# Send a normal request to verify the server didn't crash.
echo "  ⏳ Verifying server is still alive after timeout..."
sleep 1
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 --max-time 5 "$HOST/")
check_status "Server responds after CGI timeout → 200" "200" "$STATUS"

echo ""
echo "═══════════════════════════════════════════════════════"
echo "  TEST 2: Rapid CGI timeouts with POST (stress Bug #3)"
echo "  Send multiple POST requests to slow.py to accumulate stale write-fds"
echo "═══════════════════════════════════════════════════════"

for i in $(seq 1 3); do
    echo "  ⏳ POST to slow.py attempt $i..."
    STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 --max-time 10 \
      -X POST -d "$BODY" "$HOST/cgi-bin/slow.py")
    check_status "  POST slow.py #$i → 504" "504" "$STATUS"
done

echo "  ⏳ Verifying server is still alive after repeated timeouts..."
sleep 1
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 --max-time 5 "$HOST/")
check_status "Server still alive after 3 CGI timeouts → 200" "200" "$STATUS"

echo ""
echo "═══════════════════════════════════════════════════════"
echo "  TEST 3: CGI exits early while POST body is still writing (Bug #4)"
echo "  Sends large POST to crash_early.py → CGI exits before body is consumed"
echo "═══════════════════════════════════════════════════════"

# Generate a large body so the write pipe is still open when CGI exits
LARGE_BODY=$(python3 -c 'print("A" * 100000)')

echo "  ⏳ Sending large POST to crash_early.py..."
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 --max-time 10 \
  -X POST -d "$LARGE_BODY" "$HOST/cgi-bin/crash_early.py")
echo "  Got status: $STATUS (any response without crash = partial pass)"

# The key test: is the server still alive?
echo "  ⏳ Verifying server is still alive after early CGI exit..."
sleep 1
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 --max-time 5 "$HOST/")
check_status "Server responds after early CGI exit → 200" "200" "$STATUS"

echo ""
echo "═══════════════════════════════════════════════════════"
echo "  TEST 4: Rapid early-exit CGIs with POST (stress Bug #4)"
echo "  Accumulate stale write-fds from multiple early-exit CGIs"
echo "═══════════════════════════════════════════════════════"

for i in $(seq 1 5); do
    echo "  ⏳ POST to crash_early.py attempt $i..."
    curl -s -o /dev/null --http1.0 --max-time 10 \
      -X POST -d "$LARGE_BODY" "$HOST/cgi-bin/crash_early.py" &
done
wait
sleep 2

echo "  ⏳ Verifying server survives burst of early-exit CGIs..."
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 --max-time 5 "$HOST/")
check_status "Server survives burst of early-exit CGIs → 200" "200" "$STATUS"

echo ""
echo "═══════════════════════════════════════════════════════"
echo "  TEST 5: Normal CGI POST still works (sanity check)"
echo "═══════════════════════════════════════════════════════"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 --max-time 10 \
  -X POST -d "name=test&email=test@42.fr" "$HOST/cgi-bin/post_test.py")
check_status "Normal POST CGI still works → 200" "200" "$STATUS"

echo ""
echo "═══════════════════════════════════════════════════════"
echo "  RESULTS"
echo "═══════════════════════════════════════════════════════"
echo ""
echo "  Passed: $PASS"
echo "  Failed: $FAIL"
echo ""
if [ $FAIL -eq 0 ]; then
    echo "  🎉 ALL TESTS PASSED (server survived — but write-fd leak may still exist silently)"
    echo "  ⚠️  Note: use-after-free may not always crash immediately."
    echo "     Run with valgrind/ASAN for definitive proof."
else
    echo "  ⚠️  $FAIL test(s) failed — likely due to CGI write-fd cleanup bugs"
fi
echo ""
