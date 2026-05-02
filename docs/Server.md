# Server

## Overview

`Server` represents a single **listening TCP socket** bound to a specific host and port. Its sole job during startup is to create the socket, bind it to the configured address, and start listening for incoming connections. After setup, it provides `accept()` to hand new client connections to the `EventLoop`.

**Problem it solves:** The server needs to open a low-level OS socket, configure it correctly (non-blocking, reusable address), resolve the hostname, bind to the port, and start listening — all of which involve several POSIX syscalls that can fail. `Server` encapsulates all of this complexity and throws a `std::runtime_error` with a clear message on any failure, allowing `main()` to abort cleanly.

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `_fd` | `int` | The file descriptor of the listening socket |
| `_serverBlock` | `Server_block` | A copy of the config block, used for host/port during binding |

---

## Public Functions

### `Server(const Server_block& serverBlock)` — Constructor
**Purpose:** The entire setup sequence: creates the socket, binds it to the address, and begins listening. All in one constructor call.

**How it works:**
Calls in sequence:
1. `createSocket()` — creates the socket fd.
2. `bindSocket()` — resolves the host and binds to host:port.
3. `startListening()` — calls `listen()` and logs the server start event.

If any step fails, a `std::runtime_error` is thrown. The destructor always closes `_fd` if it was opened.

---

### `accept() const` → `int`
**Purpose:** Accepts a new incoming TCP connection and returns the client's file descriptor. Called by `EventLoop::handleNewClient()` when `poll()` reports the listening socket as readable.

**Problem:** When a client connects, `accept()` must be called to get a file descriptor for that specific connection. The returned fd must immediately be set to non-blocking mode, because the entire server is non-blocking — any blocking fd would stall the event loop.

**How it works:**
1. Calls `::accept(_fd, &clientAddr, &clientLen)` to get the client fd.
2. Returns `-1` (prints an error) if `accept` fails.
3. Calls `fcntl(clientFd, F_SETFL, O_NONBLOCK)` to set the client fd to non-blocking.
4. Returns `-1` (closes the fd and prints an error) if `fcntl` fails.
5. Returns the valid, non-blocking client fd on success.

---

### `getFd() const` → `int`
**Purpose:** Returns the listening socket's file descriptor. Used by `EventLoop` to register the server fd with `poll()` for `POLLIN` events.

---

## Private Functions

### `createSocket()` — Private
**Purpose:** Creates the raw TCP socket and configures it.

**How it works:**
1. Calls `socket(AF_INET, SOCK_STREAM, 0)` to create a TCP socket.
2. Calls `fcntl(_fd, F_SETFL, O_NONBLOCK)` to make it non-blocking immediately.
3. Calls `setsockopt(SOL_SOCKET, SO_REUSEADDR, ...)` so the server can restart and reuse the same port without waiting for the OS `TIME_WAIT` to expire.

---

### `bindSocket()` — Private
**Purpose:** Resolves the hostname configured in `_serverBlock.host` and binds the socket to that address and port.

**How it works:**
1. If `host == "0.0.0.0"`: fills `sockaddr_in` directly with `INADDR_ANY` (listen on all interfaces) and the configured port.
2. Otherwise: calls `getaddrinfo()` to resolve the hostname to an IP address, fills `sockaddr_in` from the result, and calls `freeaddrinfo()`.
3. Calls `bind(_fd, &serverAddr, sizeof(serverAddr))`. Throws on failure.

---

### `startListening() const` — Private
**Purpose:** Calls the OS `listen()` syscall to mark the socket as passive (ready to accept connections) and logs the event.

**How it works:**
Calls `listen(_fd, SOMAXCONN)` where `SOMAXCONN` is the maximum OS-level connection backlog. Throws on failure. Then calls `Logger::serverStart()` to print the server's address to the terminal.
