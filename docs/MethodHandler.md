# MethodHandler

## Overview

`MethodHandler` is the **core HTTP request processing engine**. It receives a fully parsed `HTTPRequest` and a `RouteConfig`, and produces a `Response` struct. It handles the three supported HTTP methods (`GET`, `POST`, `DELETE`), enforces security and permission checks, manages session cookies, and generates appropriate error responses. It also handles special sub-features like autoindex directory listings, file uploads, and CGI detection.

**Problem it solves:** Once a request is parsed and routed, the server must determine what to do with it. This involves many layers of checks: is the method allowed? Is the URI safe? Is there a redirect? Is it a CGI request? Is the target a file or directory? Does the file exist? Is it readable? `MethodHandler` encapsulates this entire decision tree for all three HTTP methods.

---

## Static Session State

```cpp
static map<string, string> AllSession;      // Active session store
static unsigned long gSessionCounter = 0;  // Monotonic counter for unique session IDs
```

These static variables persist for the lifetime of the server process, forming a simple in-memory session store.

---

## Public Functions

### `handleGET(HTTPRequest& request, const RouteConfig& route)` → `Response`
**Purpose:** Serves static files, directories, redirects, and triggers CGI detection for GET requests.

**Decision chain:**
1. **Permission check:** Calls `isSafeAndAllowed("GET", request, route, resp)`. If the method is not in the location's `allowed_methods`, returns `405`. If the URI contains `..` path traversal or `\` backslash, returns `403`.
2. **Redirect:** If `route.getRedirect()` is non-empty, returns a `301` redirect response with a `Location` header.
3. **CGI detection:** Calls `tryCGI(request, route)`. If the URI's file extension matches an entry in `cgi_pass`, sets `request.setIsCGI(true)` and returns a blank `Response` (the actual handling is done by `EventLoop` via `CGIHandler`).
4. **Path resolution:** Constructs the filesystem path from the route's root and the URI, using smart deduplication to avoid double-including path segments.
5. **Directory handling:** If the resolved path is a directory:
   - Redirects to the trailing-slash version if missing (e.g., `/dir` → `/dir/`).
   - Looks for an index file (`route.getIndex()`). If found and readable, serves it. If the index is a CGI script, triggers CGI.
   - If `autoindex` is enabled, generates a directory listing with `buildAutoindex()`.
   - Otherwise, returns `403 Forbidden`.
6. **File handling:** Checks existence → readability → reads file content → sets `Content-Type` via `getTheFileType()` → returns `200 OK`.
7. **Session:** Wraps the final response in `applySession()` before returning.

---

### `handlePOST(HTTPRequest& request, const RouteConfig& route)` → `Response`
**Purpose:** Handles file uploads and CGI POST requests.

**Decision chain:**
1. **Permission check:** Same as GET — checks `isSafeAndAllowed("POST", ...)`.
2. **Redirect:** Checks for `return` directive in config.
3. **Body size limit:** If `route.getMaxBodySize() > 0` and `body.size() > max`, returns `413 Payload Too Large`.
4. **CGI detection:** Same as GET. If CGI, sets the flag and returns early.
5. **Multipart file upload:** If `Content-Type: multipart/form-data`:
   - Creates a `FileUpload` object and calls `parseTheThing()` to extract the file data.
   - Returns `400` if parsing fails.
   - Gets the upload directory from `route.getUploadStore()`. Returns `500` if not configured or missing.
   - Calls `saveTheThing()` to write the file to disk.
   - Returns `201 Created` on success (with a `Location` header), or `500` on write failure.
6. **Generic POST body:** For `application/x-www-form-urlencoded` or raw bodies, returns `200 OK` with a simple confirmation page.
7. **Session:** Wraps response in `applySession()`.

---

### `handleDELETE(HTTPRequest& request, const RouteConfig& route)` → `Response`
**Purpose:** Deletes a file from the filesystem.

**Decision chain:**
1. **Permission check:** `isSafeAndAllowed("DELETE", ...)`.
2. **Path resolution:** Constructs the filesystem path.
3. **Directory check:** Deleting directories is not allowed — returns `403 Forbidden`.
4. **Existence check:** Returns `404 Not Found` if the file doesn't exist.
5. **Permission check:** Calls `canDeletePath()` — checks if the **parent directory** is writable/executable. Returns `403` if not.
6. **Delete:** Calls `std::remove()`. Returns `500` on failure.
7. Returns `204 No Content` (no body) on success.
8. **Session:** Wraps response in `applySession()`.

---

### `makeError(int code, const string& msg, const RouteConfig& route)` → `Response` (static)
**Purpose:** Builds an error `Response`. First tries to serve the configured custom error page from `route.getErrorPages()`. Falls back to an inline HTML error page if the file is missing.

**How it works:**
1. Looks up `code` in `route.getErrorPages()`.
2. If found: tries to read the file directly (as a path relative to the working directory).
3. If not found as-is: constructs the full path using `route.getRoot()` (stripping `cgi-bin` if present).
4. If the file can be read: returns it as the response body.
5. Falls back to a minimal inline HTML `<h1>CODE MESSAGE</h1>` page.

---

### `getTheFileType(const string& path) const` → `string`
**Purpose:** Determines the MIME type (Content-Type) based on the file extension.

**How it works:** Extracts the file extension (the substring after the last `.` that appears after the last `/`). Converts to lowercase and returns the matching MIME type from a hardcoded map covering: HTML, CSS, JS, JSON, PDF, images (JPEG, PNG, GIF, SVG, etc.), audio, video, fonts. Returns `"application/octet-stream"` for unknown extensions.

---

## Private Helper Functions

| Function | Purpose |
|---|---|
| `ensureSession(request, resp)` | Checks for `SESSION_ID` cookie; creates a new session and sets `Set-Cookie` header if none exists |
| `applySession(request, resp)` | Calls `ensureSession()` and returns the response — wraps every response |
| `isAllowed(method, route)` | Returns `true` if the method is in the location's `allowed_methods` (empty list = all allowed) |
| `isItUnsafe(uri)` | Returns `true` if the URI contains `..` segments or `\` characters (path traversal protection) |
| `makeThePath(root, uri)` | Joins root + URI into a filesystem path, avoiding double slashes |
| `readFileContent(path, content)` | Reads a file into a string using binary mode; returns `false` on failure |
| `isDirectory(path)` | Returns `true` if `stat()` says the path is a directory |
| `fileExists(path)` | Returns `true` if `stat()` succeeds for the path |
| `isReadable(path)` | Returns `true` if `access(path, R_OK) == 0` |
| `canDeletePath(path)` | Returns `true` if the parent directory has write+execute permission |
| `buildAutoindex(dirPath, uri)` | Generates an HTML directory listing; uses `autoindex.html` template if available |
| `tryCGI(request, route)` | Returns `true` if the URI's extension is in `cgi_pass` |
| `isSafeAndAllowed(method, request, route, resp)` | Combines method and path safety checks; sets `resp` on failure |
| `makeRedirect(location)` | Builds a `301 Moved Permanently` response with `Location` header |
| `RemoveQueryString(uri)` | Strips the `?query` part from a URI |
| `makeSessionId()` | Generates a unique session ID string using timestamp and counter |
