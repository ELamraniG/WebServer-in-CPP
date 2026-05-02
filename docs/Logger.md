# Logger

## Overview

`Logger` is a stateless, all-static utility class that provides **colorized, badge-formatted terminal output** for all significant server events: server startup, client connections/disconnections, static file serving, CGI execution, timeouts, and errors. All output goes to `stdout` with ANSI escape codes for color.

**Problem it solves:** A running web server produces a stream of events. Without structured logging, debugging concurrent requests is nearly impossible. `Logger` provides a consistent, human-readable format where the event type is immediately identifiable by color and label, making it easy to trace the flow of requests at a glance.

---

## Output Format

Each log line follows this general pattern:
```
[BADGE]  <detail>  [STATUS_CODE]
```
- **BADGE:** A colored background label identifying the event type (e.g., `SERVER`, `CONNECT`, `DISCONNECT`, `STATIC`, `CGI`, `TIMEOUT`, `ERROR`).
- **Detail:** Contextual information like IP, URI, fd number, or script path.
- **Status code:** A colored status indicator (e.g., `[200 OK]`, `[404 NOT FOUND]`).

Colors are rendered using 24-bit ANSI escape codes (`\033[38;2;R;G;Bm` for foreground, `\033[48;2;R;G;Bm` for background).

---

## Public Static Functions

### `serverStart(host, port, fd)`
**Purpose:** Logs when a server socket starts listening.

**Output example:**
```
 SERVER  listening on 0.0.0.0:8080  fd[4]
```

---

### `clientConnected(fd, ip, port)`
**Purpose:** Logs when a new TCP connection is accepted.

**Output example:**
```
 CONNECT  127.0.0.1:54321  fd[7]
```

---

### `clientDisconnected(fd, code)`
**Purpose:** Logs when a client connection is closed (for any reason: clean close, timeout, error).

**Output example:**
```
 DISCONNECT  fd[7]  [CLOSED]
```
or for a request that completed:
```
 DISCONNECT  fd[7]  [200 OK]
```

---

### `clientTimeout(fd)`
**Purpose:** Logs when a client connection is closed due to inactivity timeout.

**Output example:**
```
 TIMEOUT  client fd[7] took too long  [408 TIMEOUT]
```

---

### `staticFile(method, uri, code)`
**Purpose:** Logs the outcome of a static file request (GET, POST, DELETE).

**Output example:**
```
 STATIC  GET  /pages/index.html  [200 OK]
```

---

### `cgiRun(fd, script)`
**Purpose:** Logs when a CGI process is spawned.

**Output example:**
```
 CGI  spawned  fd[7]  /cgi-bin/test.py
```

---

### `cgiDone(fd, script, code)`
**Purpose:** Logs when a CGI process completes successfully.

**Output example:**
```
 CGI  done     fd[7]  /cgi-bin/test.py  [200 OK]
```

---

### `cgiTimeout(fd, script)`
**Purpose:** Logs when a CGI process is killed due to timeout.

**Output example:**
```
 CGI  timeout  fd[7]  /cgi-bin/slow.py  [504 GW TIMEOUT]
```

---

### `cgiError(fd, script, code)`
**Purpose:** Logs when a CGI process fails (non-zero exit, pipe error, etc.).

**Output example:**
```
 CGI  error    fd[7]  /cgi-bin/crash_early.py  [500 INTERNAL ERROR]
```

---

### `error(msg)`
**Purpose:** Logs a generic error message, typically for server-side errors before a response is sent.

**Output example:**
```
 ERROR  [400] BAD REQUEST
```

---

## Private Helper Functions

| Function | Purpose |
|---|---|
| `colorOf(code)` | Returns ANSI foreground color based on HTTP status class (2xx=green, 3xx=cyan, 4xx=orange, 5xx=red, 0=gray) |
| `labelOf(code)` | Returns the short label string for a status code (e.g., `"200 OK"`, `"404 NOT FOUND"`) |
| `statusStr(code)` | Combines `colorOf()` and `labelOf()` into a formatted `[CODE LABEL]` string |
| `badge(bg, label)` | Returns a bold, background-colored label string: `" LABEL "` with reset after |

---

## ANSI Color Constants (Private)

All color methods return ANSI escape code strings. Colors use 24-bit RGB for precision:

| Method | Color | Used for |
|---|---|---|
| `C_GREEN()` | `#50C878` | 2xx success |
| `C_CYAN()` | `#50B4DC` | 3xx redirects, CONNECT badge |
| `C_YELLOW()` | `#F0B432` | Timeouts |
| `C_RED()` | `#DC4646` | 5xx errors |
| `C_ORANGE()` | `#F08232` | 4xx client errors |
| `C_MAGENTA()` | `#B464DC` | CGI events |
| `C_GRAY()` | `#787882` | Disconnections, fd labels |
