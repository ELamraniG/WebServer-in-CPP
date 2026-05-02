# Client

## Overview

`Client` represents a single connected HTTP client. It owns the client socket's file descriptor and manages two buffers: one for accumulating incoming raw bytes (the request) and one for buffering the outgoing HTTP response. It also tracks the timestamp of the last activity for timeout detection.

**Problem it solves:** A single HTTP request does not arrive in one `read()` call — it can come in multiple fragments. The `Client` accumulates all incoming data in `_requestBuffer` until the `RequestParser` reports that a full, complete HTTP message has arrived. Similarly, the full response may not fit in one `write()` call, so `_responseBuffer` holds whatever hasn't been sent yet.

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `_fd` | `int` | The client socket file descriptor |
| `_requestBuffer` | `string` | Accumulates raw bytes received from the client |
| `_responseBuffer` | `string` | Holds the HTTP response waiting to be written to the socket |
| `_lastActivity` | `time_t` | Timestamp of the last read or write, used for timeout detection |
| `httpReq` | `HTTPRequest` | The parsed HTTP request object (public; filled by `RequestParser`) |

---

## Public Functions

### `Client(int fd)` — Constructor
**Purpose:** Creates a client tied to a specific socket fd. Initializes the request buffer to empty and records the current time as the last activity.

---

### `readFromSocket()` → `ssize_t`
**Purpose:** Reads available data from the client socket into `_requestBuffer`. Called by `EventLoop::handleReadEvent()` when `poll()` reports the client fd as readable (`POLLIN`).

**Problem:** The kernel has data waiting, but we don't know how much. We must read it without blocking and accumulate it across multiple calls until the `RequestParser` reports a complete request.

**How it works:**
1. Calls `read(_fd, buffer, BUFFER_SIZE)` (non-blocking read of up to 4096 bytes).
2. If `bytes > 0`: appends the data to `_requestBuffer`.
3. Returns the raw byte count (the caller checks: `< 0` = error, `== 0` = disconnected, `> 0` = data received).

---

### `writeToSocket()` → `ssize_t`
**Purpose:** Writes pending response data from `_responseBuffer` to the client socket. Called by `EventLoop::handleWriteEvent()` when `poll()` reports the client fd as writable (`POLLOUT`).

**Problem:** A large response may not fit in one `write()` call. The kernel has a limited send buffer. We write what fits and track what's left.

**How it works:**
1. Calls `write(_fd, _responseBuffer.c_str(), responseSize)`.
2. If `bytes > 0`: calls `eraseConsumedData(bytes)` to remove the written bytes from the front of `_responseBuffer`.
3. Returns the raw byte count (the caller checks: `< 0` = error, and `hasNoPendingWrite()` = fully sent).

---

### `setResponse(const string& response)`
**Purpose:** Loads the full HTTP response string into `_responseBuffer`, ready for `writeToSocket()` to send it out.

---

### `hasNoPendingWrite() const` → `bool`
**Purpose:** Returns `true` when `_responseBuffer` is empty (the entire response has been sent to the kernel). The `EventLoop` uses this to know when to close the connection.

---

### `isTimedOut() const` → `bool`
**Purpose:** Returns `true` when the client has been inactive for more than `TIMEOUT` seconds (45 seconds, defined in `ServerConstants.hpp`). The `EventLoop` checks this every iteration to detect stale connections.

**How it works:** Compares `time(NULL) - _lastActivity` against the `TIMEOUT` constant.

---

### `isRequestCompleted() const` → `bool`
**Purpose:** A quick preliminary check to see if the raw buffer contains the end-of-headers marker (`\r\n\r\n`). Returns `true` if it does, indicating that at minimum the headers have fully arrived.

> **Note:** This is a quick heuristic. The definitive completion check is done by `RequestParser::parseRequest()`.

---

### `updateLastActivity()`
**Purpose:** Resets `_lastActivity` to the current time (`time(NULL)`). Called after processing a request to prevent the connection from being timed out while a response is being prepared.

---

### `eraseConsumedData(int bytes)`
**Purpose:** Removes the first `bytes` characters from `_responseBuffer` (called internally by `writeToSocket()`). This slides the buffer forward, so the next `writeToSocket()` call sends the next chunk.

---

### `getRequestBuffer()` → `string&`
**Purpose:** Returns a reference to `_requestBuffer`. Used by `EventLoop` to pass the raw bytes to `RequestParser::parseRequest()`.

---

### `getFd() const` → `int`
**Purpose:** Returns the client's socket fd. Used by `EventLoop` to manage the `poll()` set and to log events.

---

### Destructor `~Client()`
**Purpose:** Closes the socket file descriptor when the `Client` object is destroyed, ensuring no file descriptor leaks.
