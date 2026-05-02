# EventLoop

## Overview

`EventLoop` is the **central nervous system** of the web server. It is a single-threaded, non-blocking multiplexed event loop built on top of `poll()`. It manages all file descriptors simultaneously — server listening sockets, client sockets, and CGI pipe file descriptors — and dispatches events to the appropriate handler functions. It runs forever until the server receives a shutdown signal (`g_running = false`).

**Problem it solves:** A web server must handle many concurrent clients without using one thread per client (which wastes resources). The solution is I/O multiplexing: use `poll()` to wait until *any* file descriptor becomes ready, then handle that specific fd. This allows one thread to manage hundreds of concurrent connections efficiently.

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `_pollFds` | `vector<pollfd>` | The array passed to `poll()`. Each entry is an fd + events mask + returned events mask |
| `_serverList` | `vector<Server*>` | The listening server sockets |
| `_serverBlocks` | `vector<Server_block>` | The raw config data, used for routing |
| `_listeningFds` | `set<int>` | Set of server fd numbers, for quick O(log n) lookup |
| `_clientMap` | `map<int, Client*>` | Maps client fd → `Client*` object |
| `_cgiFdToHandler` | `map<int, CGIHandler*>` | Maps a CGI pipe fd → its `CGIHandler` |
| `_cgiFdToClient` | `map<int, Client*>` | Maps a CGI pipe fd → the `Client` that made the CGI request |
| `_cgiStartTime` | `map<int, time_t>` | Maps CGI read fd → the time the CGI was started, for timeout tracking |

---

## Public Functions

### `EventLoop(servers, serverBlocks)` — Constructor
**Purpose:** Registers all listening server sockets with `poll()` for `POLLIN` events.

**How it works:**
Iterates over `serverList`, inserts each server's fd into `_listeningFds`, and calls `addToPoll(fd, POLLIN)` for each.

---

### `run()`
**Purpose:** The main infinite loop. Drives the entire server.

**How it works:**
1. Calls `poll(_pollFds.data(), _pollFds.size(), POLL_TIMEOUT)`.
2. Iterates over all fds in `_pollFds` and for each fd:
   - Checks `isCGITimeout(fd)` → calls `handleCGITimeout()`.
   - Checks `isTimeout(fd)` (client idle) → calls `handleClientDisconnected()` with `408`.
   - Checks `isError(fd, revents)` → calls `handleClientDisconnected()` with `500`.
   - Checks `isReadable(revents)` → calls `handleReadEvent()`.
   - Checks `isWritable(revents)` → calls `handleWriteEvent()`.

---

## Private Functions

### `handleReadEvent(int fd, size_t& i)`
**Purpose:** Dispatches readable fd events to the right sub-handler.

**How it works:**
- If `isServer(fd)`: it's a listening socket, call `handleNewClient(fd)`.
- Else if the fd is in `_cgiFdToHandler`: it's a CGI output pipe, call `handleCGIRead(fd, i)`.
- Otherwise: it's a regular client socket. Reads with `client->readFromSocket()`. If `bytes == 0`, the client disconnected cleanly. If `bytes < 0`, it's a socket error. If `bytes > 0`, passes the buffer to `RequestParser::parseRequest()` and checks the result:
  - `P_INCOMPLETE`: returns and waits for more data.
  - `P_ERROR`: sends a `400 Bad Request` response.
  - `P_SUCCESS`: calls `handleRequestComplete()`.

---

### `handleWriteEvent(int fd, size_t& i)`
**Purpose:** Dispatches writable fd events.

**How it works:**
- If `fd` is in `_cgiFdToHandler`: it's a CGI input pipe, call `handleCGIWrite()`.
- Otherwise: it's a client socket. Calls `client->writeToSocket()`. If `bytes < 0`, it's an error → disconnect. If `hasNoPendingWrite()` is true, the response is fully sent → disconnect cleanly.

---

### `handleNewClient(int serverFd)`
**Purpose:** Accepts a new TCP connection from the listening socket and registers it with `poll()`.

**How it works:**
1. Finds the `Server` object matching `serverFd`.
2. Calls `server->accept()` to get the new client fd (already set to non-blocking by `Server::accept()`).
3. Logs the connection with `Logger::clientConnected()`.
4. Calls `addToPoll(clientFd, POLLIN)` to watch it for incoming data.
5. Creates a new `Client` object and stores it in `_clientMap[clientFd]`.

---

### `handleRequestComplete(int fd, size_t& i, const RouteConfig& route)`
**Purpose:** Called when `RequestParser` reports a complete HTTP request. Dispatches to the correct HTTP method handler.

**How it works:**
1. Gets the HTTP method from the parsed request.
2. Dispatches to `handler.handleGET()`, `handler.handlePOST()`, or `handler.handleDELETE()`.
3. For any unknown method: responds with `501 Not Implemented`.
4. If the request is CGI (detected by `MethodHandler` setting `request.setIsCGI(true)`): calls `startCGI()`. Sets the client fd's events to `PAUSE` (0) while waiting for the CGI to finish.
5. For static requests: builds the response immediately with `ResponseBuilder::build()`, sets the client fd's events to `POLLOUT`.

---

### `startCGI(int clientFd, const RouteConfig& route)` → `bool`
**Purpose:** Creates a `CGIHandler`, launches the CGI process, and registers the pipe fds with `poll()`.

**How it works:**
1. Calls `resolveCGI()` to find the script's absolute filesystem path.
2. Creates a `CGIHandler` with the request's method, query string, body, and headers.
3. Calls `cgi->start()`. If this fails (404, 403, 500), sets the client's response to the error and returns `false`.
4. On success: registers `cgi->getReadFd()` with `POLLIN` and (if POST) `cgi->getWriteFd()` with `POLLOUT`.
5. Stores the CGI handler in `_cgiFdToHandler` and the client in `_cgiFdToClient`, keyed by both pipe fds.
6. Records `_cgiStartTime[readFd] = time(NULL)`.

---

### `handleCGIRead(int readFd, size_t& i)`
**Purpose:** Called when the CGI script's output pipe is readable. Reads available output and, when done, builds the response.

**How it works:**
1. Calls `cgi->readOutput()` to append new data to the handler's output buffer.
2. If `cgi->isDone()` or `cgi->isError()`:
   - On error: sets a `500` response on the client.
   - On success: calls `ResponseBuilder::buildCgiResponse(cgi->getOutput(), route)` and sets it on the client.
   - Switches the client fd's events to `POLLOUT`.
   - Cleans up: removes the read fd from `_pollFds`, deletes the `CGIHandler`, erases all related map entries.

---

### `handleCGIWrite(int writeFd, size_t& i)`
**Purpose:** Called when the CGI script's input pipe is writable. Feeds the POST body to the script.

**How it works:**
1. If the CGI is in error or the body is fully written: removes the write fd from `_pollFds` and map entries.
2. Otherwise: calls `cgi->writeBody()` to send the next chunk of the request body to the script's stdin.

---

### `handleCGITimeout(int fd, size_t& i)`
**Purpose:** Called when a CGI script has been running for more than `CGI_TIMEOUT` seconds (5s). Kills the process and returns `504 Gateway Timeout` to the client.

**How it works:**
1. Logs the timeout.
2. Sets the client's events to `POLLOUT`.
3. Calls `cgi->cleanup()` (sends SIGKILL, waits for the child, closes pipes).
4. Sets the client's response to a `504` error.
5. Deletes the `CGIHandler` and removes all related map entries and poll fds.

---

### `handleClientDisconnected(int fd, size_t& i, HttpStatus code)`
**Purpose:** Cleans up and removes a client connection.

**How it works:**
Logs the disconnection, deletes the `Client` object, erases it from `_clientMap`, and removes its fd from `_pollFds` (via `removeFromPoll`).

---

### `getRoute(const Client* client)` → `RouteConfig`
**Purpose:** A convenience helper that runs the full routing logic (match server block, then match location block) and returns a `RouteConfig` object for the given client's request. Used by multiple handlers to get route context.

---

### Utility Predicates
| Function | Returns `true` when... |
|---|---|
| `isServer(fd)` | `fd` is in `_listeningFds` |
| `isReadable(revents)` | `revents` has `POLLIN` or `POLLHUP` |
| `isWritable(revents)` | `revents` has `POLLOUT` |
| `isTimeout(fd)` | `fd` is a client and `client->isTimedOut()` |
| `isCGITimeout(fd)` | `fd` is a CGI fd and has been running > `CGI_TIMEOUT` seconds |
| `isError(fd, revents)` | `fd` is a client (not a CGI pipe or server) and `revents` has `POLLERR`/`POLLHUP` |
