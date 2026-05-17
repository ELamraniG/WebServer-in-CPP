# Server

## Overview

`Server` represents a single listening TCP socket bound to a host/port as specified in one `Server_block` config entry. It owns the socket via a `SocketGuard` (RAII wrapper) and is responsible for the full socket setup lifecycle: create → bind → listen. Once running, its only ongoing role is accepting new connections.

**Problem it solves:** Isolates all the low-level socket setup (`socket()`, `setsockopt()`, `bind()`, `listen()`) and address resolution behind a clean object. `EventLoop` only needs to call `getFd()` to poll it and `accept()` to get new clients — it never touches socket syscalls directly.

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `_socket` | `SocketGuard` | RAII wrapper that owns the listening socket fd and closes it on destruction |
| `_config` | `Server_block` | The parsed config block for this server (host, port, server names, locations, etc.) |

> `Server` is non-copyable. Copy constructor and assignment operator are private and unimplemented.

---

## Constructor & Destructor

### `Server(const Server_block& serverBlock)`
Stores the config, then runs the full socket setup sequence:
1. `createSocket()` — allocates the TCP socket fd.
2. `buildAddress()` — resolves the bind address.
3. `bindSocket()` — binds to the address.
4. `startListening()` — calls `listen()` to accept incoming connections.

Throws on any failure so that `ServerFactory` can abort early rather than run with a broken server.

### `~Server()`
`SocketGuard` destructor closes the fd automatically.

---

## Public Methods

### `accept()` → `int`
**Purpose:** Accepts one pending connection from the listening socket.

**Returns:** The new client fd, already set to non-blocking mode (`O_NONBLOCK`). Returns `-1` on failure (e.g., `EAGAIN` if no connection is pending).

The event loop calls this when `poll()` reports `POLLIN` on this server's fd.

---

### `getFd()` → `int`
Returns `_socket.fd` — the listening socket file descriptor registered with `poll()`.

---

### `getConfig()` → `const Server_block&`
Returns the server's parsed config. Used by `EventLoop` and the routing logic to access server names, locations, and limits for the server that accepted a given client.

---

## Private Methods

### `createSocket()`
Calls `socket(AF_INET, SOCK_STREAM, 0)` and sets `SO_REUSEADDR` to avoid `bind()` failures on restart. Stores the fd in `_socket.fd`.

### `buildAddress()` → `struct sockaddr_in`
Delegates to `resolveAddress()` or `buildAnyAddress()` depending on whether the config specifies a specific host or `0.0.0.0`.

### `buildAnyAddress(int port)` → `struct sockaddr_in`
Builds a `sockaddr_in` with `INADDR_ANY` and the configured port. Used when the server should accept connections on all interfaces.

### `resolveAddress(const char* host, const char* port)` → `struct sockaddr_in`
Resolves a specific hostname to an IP using `getaddrinfo()`. Used when the config specifies a non-wildcard host.

### `bindSocket()`
Calls `bind()` with the resolved address. Throws on failure.

### `startListening() const`
Calls `listen()` with `SOMAXCONN`. Throws on failure.