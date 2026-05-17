# Client

## Overview

`Client` represents a single connected TCP client. It wraps the client socket fd, holds references to the `Server` that accepted it, and owns the read/write buffers for that connection. It also exposes a public `HTTPRequest` object that gets populated by `RequestParser` as data arrives.

**Problem it solves:** The event loop needs to track per-connection state across multiple `poll()` iterations — partial request data, pending response data, idle timeout, the parsed request object, and which server config applies. `Client` is that state container. It never drives logic itself; it exposes data and simple I/O primitives that the event loop calls.

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `_fd` | `int` | The accepted client socket file descriptor |
| `_server` | `Server*` | Pointer to the `Server` that accepted this connection (provides config access) |
| `_requestBuffer` | `std::string` | Raw bytes read from the socket, accumulated until a full request is parsed |
| `_responseBuffer` | `std::string` | The full HTTP response, drained to the socket on write events |
| `_lastActivity` | `time_t` | Timestamp of the last read/write activity, used for idle timeout detection |
| `httpReq` | `HTTPRequest` | **Public.** The parsed HTTP request object. Written by `RequestParser`, read by method handlers and `EventLoop` |

> `Client` is non-copyable. Copy constructor and assignment operator are private and unimplemented.

---

## Constructor & Destructor

### `Client(int fd, Server* server)`
Stores the fd and server pointer, initializes `_lastActivity` to `time(NULL)`. Does **not** take ownership of the `Server*` — it is not deleted on destruction.

### `~Client()`
Closes `_fd` via `close()`.

---

## Public Methods

### `getFd()` → `int`
Returns the client socket fd.

---

### `getServer()` → `const Server*`
Returns the server that accepted this client. Used by the event loop to retrieve server config (host, port, server blocks) for routing.

---

### `readFromSocket()` → `ssize_t`
**Purpose:** Reads up to `BUFFER_SIZE` bytes from the socket into `_requestBuffer`.

**Returns:**
- `> 0`: bytes read successfully; data appended to `_requestBuffer`
- `0`: client closed the connection cleanly (EOF)
- `< 0`: socket error

The event loop checks the return value to decide whether to parse, disconnect, or wait.

---

### `writeToSocket()` → `ssize_t`
**Purpose:** Writes as much of `_responseBuffer` as the socket will accept in one call.

**Returns:**
- `> 0`: bytes written; the written portion is removed from `_responseBuffer`
- `< 0`: socket error

The event loop calls this repeatedly on `POLLOUT` until `hasNoPendingWrite()` returns true.

---

### `hasNoPendingWrite()` → `bool`
Returns `true` when `_responseBuffer` is empty — i.e., the full response has been flushed. The event loop uses this to know when to close or recycle the connection.

---

### `isTimedOut()` → `bool`
Returns `true` when `time(NULL) - _lastActivity > TIMEOUT` (45 seconds). The event loop checks this on every iteration to send a `408 Request Timeout` and close idle connections.

---

### `isRequestCompleted()` → `bool`
Returns `true` when the HTTP request in `httpReq` is fully parsed and ready to dispatch. Delegates to `HTTPRequest`'s completion state.

---

### `eraseConsumedData(int bytes)`
Removes the first `bytes` characters from `_requestBuffer` after the parser has consumed them. Prevents double-parsing of already-processed data.

---

### `updateLastActivity()`
Sets `_lastActivity = time(NULL)`. Called by the event loop after any successful read or write to reset the idle timeout clock.

---

### `setResponse(const std::string& response)`
Copies `response` into `_responseBuffer`. Called by the event loop once a response has been built (static or CGI), right before switching the fd's poll event to `POLLOUT`.

---

### `getRequestBuffer()` → `std::string&`
Returns a reference to the raw request accumulation buffer. Used by `RequestParser` to read and parse incoming data.