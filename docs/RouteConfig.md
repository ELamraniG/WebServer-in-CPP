# RouteConfig

## Overview

`RouteConfig` is a **read-only view** that merges a `Server_block` and an optional `location_block` into a single, unified configuration interface. It implements the **inheritance rule**: if a `location_block` defines a value (like `root` or `index`), use that; otherwise fall back to the `Server_block`'s value.

**Problem it solves:** The `MethodHandler`, `EventLoop`, and other components need to query configuration values without knowing whether the value came from a `server` block or a `location` block. `RouteConfig` abstracts this away, implementing the fallback logic in one place.

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `_server` | `const Server_block&` | Reference to the matched server configuration |
| `_location` | `const location_block*` | Pointer to the matched location (or `NULL` if no location matched) |
| `_allowed_methods_cache` | `mutable vector<string>` | Lazy-built cache of allowed method names |

---

## Public Functions

### `RouteConfig(const Server_block& s, const location_block* l)` — Constructor
**Purpose:** Stores references to the matched server block and location block (which may be `NULL`).

---

### `getRoot() const` → `const string&`
**Purpose:** Returns the root filesystem path for serving files.

**How it works:** Returns `_location->root` if the location exists and has a non-empty root; otherwise falls back to `_server.root`.

---

### `getIndex() const` → `const string&`
**Purpose:** Returns the default index filename (e.g., `"index.html"`).

**How it works:** Returns `_location->index` if set; otherwise `_server.index`.

---

### `getAutoindex() const` → `bool`
**Purpose:** Returns whether directory listing (autoindex) is enabled.

**How it works:** Returns `_location->autoindex` if a location exists; otherwise `false` (autoindex is always location-specific in this server).

---

### `getAllowedMethods() const` → `const vector<string>&`
**Purpose:** Returns the list of allowed HTTP methods for this route.

**Problem:** The raw config stores methods as `map<string, bool>` (e.g., `{"GET": true, "POST": true}`). Callers need a simple list of allowed method names. An empty list means all methods are allowed.

**How it works:** If a location exists, iterates over `_location->methods` and pushes every method where `value == true` into `_allowed_methods_cache`. Returns the cache. If no location, returns an empty vector (no restrictions).

---

### `getRedirect() const` → `const string&`
**Purpose:** Returns the redirect target URL if a `return` directive is configured.

**How it works:** Returns the URL string from the first (and only) entry in `_location->redirect`. Returns an empty string reference if no redirect is set.

---

### `getRedirectCode() const` → `int`
**Purpose:** Returns the HTTP redirect status code (301, 302, etc.) or `0` if no redirect is configured.

---

### `getUploadStore() const` → `const string&`
**Purpose:** Returns the directory where uploaded files should be saved (from `upload_pass` directive).

---

### `getErrorPages() const` → `const map<int, string>&`
**Purpose:** Returns the map of custom error page paths from the **server block** (error pages are always server-level, not location-level).

---

### `getMaxBodySize() const` → `size_t`
**Purpose:** Returns `client_max_body_size` from the server block (in bytes). Used by `MethodHandler::handlePOST()` to reject oversized uploads with `413`.

---

### `getCgiPass() const` → `const map<string, string>&`
**Purpose:** Returns the CGI extension-to-interpreter map from the location block (e.g., `{".py": "/usr/bin/python3"}`). Returns an empty map if no location is set.

---

### `getLocationPath() const` → `const string&`
**Purpose:** Returns the matched location's path string (e.g., `"/cgi-bin"`). Used by `EventLoop::resolveCGI()` to strip the location prefix when computing a script's filesystem path.
