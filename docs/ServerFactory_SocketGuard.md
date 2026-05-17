# ServerFactory

## Overview

`ServerFactory` provides two free functions that manage the lifetime of all `Server` objects. It is the only place where `Server` instances are created and destroyed, keeping `main()` clean of allocation logic.

---

## Functions

### `createServers(vector<Server_block>& serverBlocks)` → `vector<Server*>`
**Purpose:** Instantiates one `Server` per `Server_block` and returns the list.

**How it works:**
Iterates over `serverBlocks`, constructs a `new Server(block)` for each, and appends it to the result vector. If any `Server` constructor throws (e.g., `bind()` fails because the port is already in use), the exception propagates and the partially-built list is not returned — caller should destroy what was allocated.

Called once at startup before `EventLoop` is constructed.

---

### `destroyServers(vector<Server*>& serverList)`
**Purpose:** Deletes all `Server` objects and clears the vector.

Called at shutdown to cleanly close all listening sockets before the process exits.

---

---

# SocketGuard

## Overview

`SocketGuard` is a minimal RAII wrapper around a single socket file descriptor. Its only job is to ensure the fd is closed when the owning object is destroyed, preventing fd leaks even when exceptions are thrown during `Server` construction.

**Problem it solves:** Raw `int fd` members don't close themselves. Without a guard, an exception thrown after `socket()` but before the `Server` destructor runs would leak the fd. `SocketGuard` closes it unconditionally in its destructor.

---

## Members

| Member | Type | Purpose |
|---|---|---|
| `fd` | `int` | The raw socket file descriptor. Public for direct access by `Server` |

> `SocketGuard` is non-copyable. Copy constructor and assignment operator are private and unimplemented — an fd must not be owned by two objects simultaneously.

---

## Constructor & Destructor

### `SocketGuard()`
Initializes `fd` to `-1` (an invalid fd sentinel).

### `~SocketGuard()`
If `fd >= 0`, calls `close(fd)`. Safe to call even if `fd` was never assigned a real socket (the `-1` default prevents a spurious `close()` call).