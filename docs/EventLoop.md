# EventLoop

## Overview

`EventLoop` is the **central nervous system** of the web server. It is a single-threaded, non-blocking multiplexed event loop built on top of `poll()`. It manages all file descriptors simultaneously — server listening sockets, client sockets, and CGI pipe file descriptors — and dispatches events to the appropriate handler functions. It runs until the process receives a shutdown signal.

**Problem it solves:** A web server must handle many concurrent clients without one thread per client. The solution is I/O multiplexing: use `poll()` to wait until *any* fd becomes ready, then handle only that fd. One thread manages hundreds of concurrent connections efficiently.

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `_pollFds` | `vector<pollfd>` | The array passed to `poll()` each iteration |
| `_serverList` | `vector<Server*>` | All listening `Server` objects |
| `_listeningFds` | `set<int>` | Fast O(log n) lookup to identify server fds |
| `_clientMap` | `map<int, Client*>` | Maps client fd → `Client*` |
| `_cgiFdToHandler` | `map<int, CGIHandler*>` | Maps CGI pipe fd → its `CGIHandler` |
| `_cgiFdToClient` | `map<int, Client*>` | Maps CGI pipe fd → the `Client` that spawned it |
| `_cgiStartTime` | `map<int, time_t>` | Maps CGI read fd → start time for timeout detection |

> `EventLoop` is non-copyable. Copy constructor and assignment operator are private and unimplemented.

---

## Constructor & Destructor

### `EventLoop(const vector<Server*>& servers)`
Registers all server listening fds into `_listeningFds` and calls `addToPoll(fd, POLLIN)` for each. After this, the loop is ready to call `run()`.

### `~EventLoop()`
Deletes all `Client*` objects in `_clientMap` and all `CGIHandler*` objects in `_cgiFdToHandler`.

---

## Public Methods

### `run()`
**Purpose:** The main infinite loop. Drives the entire server.

**How it works:**
1. Calls `poll(_pollFds.data(), _pollFds.size(), POLL_TIMEOUT)` (5 second timeout).
2. Delegates all fd iteration to `processEvents()`.
3. Runs until an external shutdown signal is received.

---

## Private Methods

### `processEvents()`
**Purpose:** Iterates over `_pollFds` after each `poll()` call and routes each ready fd to the correct handler.

**How it works:**
For each fd, in order:
- `isCGITimeout(fd)` → `handleCGITimeout()`
- `isClientTimeout(fd)` → `handleClientDisconnected()` with `408`
- `isError(fd, revents)` → `handleClientDisconnected()` with `500`
- `isReadable(revents)` → `handleReadEvent()`
- `isWritable(revents)` → `handleWriteEvent()`

---

### `handleReadEvent(int fd, size_t& i)`
**Purpose:** Dispatches readable fd events to the correct sub-handler.

**How it works:**
- If `isServer(fd)`: call `handleNewClient(fd)`.
- Else if fd is in `_cgiFdToHandler`: call `handleCGIRead(fd, i)`.
- Otherwise: call `handleClientRead(fd, i)`.

---

### `handleClientRead(int fd, size_t& i)`
**Purpose:** Reads from a client socket and drives request parsing.

**How it works:**
1. Calls `client->readFromSocket()`.
2. `0` → client disconnected cleanly → `handleClientDisconnected()`.
3. `< 0` → socket error → `handleClientDisconnected()`.
4. `> 0` → calls `checkRequestParsing()`.

---

### `checkRequestParsing(Client* client, size_t& i)` → `bool`
**Purpose:** Passes buffered data to `RequestParser` and acts on the result.

**Returns:** `true` if parsing is ongoing or complete, `false` on a fatal parse error.

**How it works:**
- `P_INCOMPLETE`: returns and waits for more data.
- `P_ERROR`: sends `400 Bad Request`, returns `false`.
- `P_SUCCESS`: calls `handleRequestComplete()`.

---

### `handleWriteEvent(int fd, size_t& i)`
**Purpose:** Dispatches writable fd events.

**How it works:**
- If fd is in `_cgiFdToHandler`: call `handleCGIWrite(fd, i)`.
- Otherwise: call `client->writeToSocket()`. On error → disconnect. If `hasNoPendingWrite()` → disconnect cleanly.

---

### `handleNewClient(int serverFd)`
**Purpose:** Accepts a new TCP connection and registers it with `poll()`.

**How it works:**
1. Finds the `Server` matching `serverFd`.
2. Calls `server->accept()` → new non-blocking client fd.
3. Calls `addToPoll(clientFd, POLLIN)`.
4. Creates a `Client` and stores it in `_clientMap[clientFd]`.

---

### `handleRequestComplete(int fd, size_t i, const RouteConfig& routeConfig)`
**Purpose:** Called when a full HTTP request is parsed. Dispatches to method handler and decides between static response and CGI.

**How it works:**
1. Calls `dispatchMethod()` to run `GET`, `POST`, or `DELETE` logic.
2. Calls `handleCGIIfNeeded()`. If true, CGI was launched — client fd is paused with event mask `PAUSE` (0) while waiting.
3. For static requests: calls `ResponseBuilder::build()`, sets response on client, switches fd to `POLLOUT`.

---

### `dispatchMethod(Client* client, const RouteConfig& route)` → `Response`
**Purpose:** Routes the request to the correct method handler based on the HTTP verb.

Returns `501 Not Implemented` for unknown methods.

---

### `handleCGIIfNeeded(int fd, size_t i, const RouteConfig& route)` → `bool`
**Purpose:** Checks if the dispatched request is a CGI request and, if so, launches it.

**Returns:** `true` if CGI was started (caller should not build a static response), `false` otherwise.

---

### `startCGI(int clientFd, const RouteConfig& route)` → `bool`
**Purpose:** Creates a `CGIHandler`, launches the CGI process, and registers pipe fds with `poll()`.

**How it works:**
1. Calls `resolveCGI()` for the script's absolute path.
2. Creates a `CGIHandler` with method, query string, body, and headers.
3. Calls `cgi->start()`. On failure (404/403/500), sets the error response on the client and returns `false`.
4. On success: registers `cgi->getReadFd()` with `POLLIN`; if POST, registers `cgi->getWriteFd()` with `POLLOUT`.
5. Stores handler and client in `_cgiFdToHandler` and `_cgiFdToClient`, records `_cgiStartTime[readFd]`.
6. Calls `registerCGI()` to centralize map insertions.

---

### `registerCGI(int fd, CGIHandler* cgi)`
**Purpose:** Inserts the CGI handler and associated client into all relevant maps for a given pipe fd. Extracted to avoid duplication between read and write fd registration.

---

### `handleCGIRead(int readFd, size_t& i)`
**Purpose:** Reads CGI stdout and, when the process is done, builds the response.

**How it works:**
1. Calls `cgi->readOutput()`.
2. If `cgi->isDone()` or `cgi->isError()`:
   - Error → sets `500` on client.
   - Success → calls `ResponseBuilder::buildCgiResponse()` and sets response on client.
   - Switches client fd to `POLLOUT`.
   - Calls `cgiCleanup()`.

---

### `handleCGIWrite(int writeFd, size_t& i)`
**Purpose:** Feeds the POST body to the CGI process stdin.

**How it works:**
- If CGI is in error or body is fully written: calls `cleanupCGIWriteFd()`.
- Otherwise: calls `cgi->writeBody()` to send the next chunk.

---

### `cleanupCGIWriteFd(int writeFd)`
**Purpose:** Removes the write pipe fd from `_pollFds` and clears its map entries. Called when the stdin feed is complete or the CGI has already errored.

---

### `cgiCleanup(int fd, size_t& i)`
**Purpose:** Full cleanup after a CGI completes or errors. Removes the read fd from `_pollFds`, deletes the `CGIHandler`, and erases all related map entries (`_cgiFdToHandler`, `_cgiFdToClient`, `_cgiStartTime`).

---

### `handleCGITimeout(int fd, size_t& i)`
**Purpose:** Kills a CGI process that has exceeded `CGI_TIMEOUT` (3 seconds) and returns `504 Gateway Timeout`.

**How it works:**
1. Sets client fd events to `POLLOUT`.
2. Calls `cgi->cleanup()` (SIGKILL + waitpid + close pipes).
3. Sets `504` response on client.
4. Calls `cgiCleanup()`.

---

### `handleClientDisconnected(int fd, size_t& i, HttpStatus code)`
**Purpose:** Tears down a client connection.

**How it works:**
Logs the disconnection, deletes the `Client` from `_clientMap`, and calls `removeFromPoll()`.

---

### `getRoute(const Client* client)` → `RouteConfig`
**Purpose:** Runs the full routing logic (match server block → match location block) for the client's request. Returns a `RouteConfig` used by handlers throughout the dispatch chain.

---

### `addToPoll(int fd, short event)`
Appends a new `pollfd` entry to `_pollFds` with the given fd and event mask.

### `removeFromPoll(size_t& i)`
Removes the entry at index `i` from `_pollFds` via swap-and-pop. Decrements `i` so the caller's loop doesn't skip the swapped-in entry.

### `changeEvent(int fd, short event)`
Finds the `pollfd` for `fd` in `_pollFds` and updates its `events` mask. Used to switch between `POLLIN`, `POLLOUT`, and `PAUSE` (0).

---

### Utility Predicates

| Function | Returns `true` when... |
|---|---|
| `isServer(fd)` | `fd` is in `_listeningFds` |
| `isReadable(revents)` | `revents` has `POLLIN` or `POLLHUP` |
| `isWritable(revents)` | `revents` has `POLLOUT` |
| `isClientTimeout(fd)` | `fd` is a client and `client->isTimedOut()` returns true |
| `isCGITimeout(fd)` | `fd` is a CGI read fd and has been running > `CGI_TIMEOUT` seconds |
| `isError(fd, revents)` | `fd` is a client (not a CGI pipe or server) and `revents` has `POLLERR` or `POLLHUP` |