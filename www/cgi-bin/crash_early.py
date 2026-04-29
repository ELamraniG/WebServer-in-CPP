#!/usr/bin/env python3
# This CGI exits immediately WITHOUT reading stdin.
# When used with a large POST body, the write pipe is still open
# while the read pipe gets EOF → triggers the write-fd cleanup bug.
import sys
sys.exit(1)
