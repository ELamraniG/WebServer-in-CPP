# Router

## Overview

`Router` is a stateless utility class with two static methods that implement the **URL routing logic**: given an incoming HTTP request, find the right `Server_block` and `location_block` from the configuration. The results are used to build a `RouteConfig` that drives all subsequent request handling decisions.

**Problem it solves:** A single server process may handle multiple virtual hosts (different `server` blocks with different ports or host names). And within each server, there may be multiple `location` blocks each matching a different URI prefix. `Router` implements the matching algorithms that map an incoming request to the correct configuration.

---

## Public Functions (all `static`)

### `match_location(const HTTPRequest& req, Server_block& server)` → `location_block*`
**Purpose:** Finds the most specific `location_block` whose `path` prefix matches the request URI.

**Problem:** Multiple location blocks can match the same URI (e.g., `/` matches everything, `/api` matches API routes). The correct one is the **longest prefix match** — the most specific location wins. This is the same algorithm Nginx uses.

**How it works:**
1. Extracts the URI from the request and strips the query string (everything from `?` onward).
2. Initializes `best_loc = NULL` and `best_len = 0`.
3. Iterates over all `location_block`s in the server:
   - **`is_prefix(uri, loc.path)`:** Checks if `loc.path` is a prefix of the URI. Skips if not.
   - **`is_segment_boundary(uri, loc.path)`:** Ensures the match is at a path segment boundary. For example, location `/api` should match `/api/users` but NOT `/apitest`. This check passes if:
     - The URI exactly equals the location path.
     - The location path is `"/"` (matches everything).
     - The location path ends with `/`.
     - The next character in the URI after the location path is `/`.
   - If both checks pass and the location's path is longer than the current best: updates `best_loc` and `best_len`.
4. Returns the `best_loc` (or `NULL` if no location matched, which causes `RouteConfig` to fall back to server-level defaults).
