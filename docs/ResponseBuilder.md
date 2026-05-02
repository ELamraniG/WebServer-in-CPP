# ResponseBuilder

## Overview

`ResponseBuilder` takes a `Response` struct (or a raw CGI output string) and serializes it into a complete, standards-compliant HTTP/1.0 response string ready to be written to the client socket.

**Problem it solves:** An HTTP response is not just a body — it requires a precise format: status line, headers (Date, Server, Content-Type, Content-Length, Connection), an empty line separator (`\r\n`), and then the body. Missing or malformed headers cause browsers to fail silently. `ResponseBuilder` centralizes this serialization logic so every response from the server follows the same correct format.

---

## Public Functions

### `build(const Response& resp) const` → `string`
**Purpose:** Serializes a `Response` struct into a full HTTP response string.

**How it works:**
1. **Status line:** `HTTP/1.0 <statusCode> <reasonPhrase>\r\n`
2. **Date header:** `Date: <RFC 1123 timestamp>\r\n` — uses `httpDate()` to format the current UTC time.
3. **Server header:** `Server: webserv/1.0\r\n`
4. **Custom headers:** Iterates over `resp.headers`. Skips any header whose key (lowercased) is `content-type` or `content-length` — these are always written explicitly below to avoid duplicates.
5. **Content-Type and Content-Length:** Written explicitly (unless status is `204 No Content` or `304 Not Modified` — those must have no body headers per HTTP spec).
6. **Connection header:** `Connection: close\r\n` — the server always closes the connection after each response (HTTP/1.0 behavior).
7. **Empty line:** `\r\n` — the mandatory separator between headers and body.
8. **Body:** Appended if status is not `204` or `304`.

---

### `buildError(int code) const` → `string`
**Purpose:** A convenience method that creates a minimal error `Response` inline and calls `build()` on it. Used when no route context is available.

**How it works:**
1. Creates a `Response` with `statusCode = code`, `contentType = "text/html"`.
2. Calls `reasonPhrase(code)` to get the human-readable reason string.
3. Builds a minimal HTML body: `<h1>CODE Reason</h1>`.
4. Calls `build(resp)` and returns the result.

---

### `buildCgiResponse(string cgiResult, RouteConfig& Route) const` → `string`
**Purpose:** Wraps the raw output of a CGI script (which already contains its own headers and body) into a proper HTTP/1.0 response.

**Problem:** CGI scripts output their own headers (like `Content-Type: text/html\r\n`) followed by a blank line and then the body. The server needs to prepend the HTTP status line, `Date`, and `Server` headers, then relay the CGI script's own headers and body verbatim.

**How it works:**
1. Writes the status line: `HTTP/1.0 200 OK\r\n`.
2. Writes `Date:` and `Server:` headers.
3. Finds `\r\n\r\n` in `cgiResult` — this separates the CGI's headers from the CGI's body. If not found, returns a `500 Internal Server Error` response.
4. Iterates over the CGI's header lines (split on `\r\n`) and writes each one to the output, preserving the original CGI headers (including `Content-Type`, `Set-Cookie`, etc.).
5. Writes a final `\r\n` (blank line separator).
6. Appends the CGI body.

---

## Private Functions

### `reasonPhrase(int code)` → `string` (static)
**Purpose:** Maps an HTTP status code integer to its standard reason phrase string.

**How it works:** A `switch` statement covering the common status codes:
- `200` → `"OK"`, `201` → `"Created"`, `204` → `"No Content"`
- `301` → `"Moved Permanently"`, `302` → `"Found"`
- `400` → `"Bad Request"`, `403` → `"Forbidden"`, `404` → `"Not Found"`, `405` → `"Method Not Allowed"`, `408` → `"Request Timeout"`, `413` → `"Payload Too Large"`, `414` → `"URI Too Long"`
- `500` → `"Internal Server Error"`, `501` → `"Not Implemented"`, `502` → `"Bad Gateway"`, `504` → `"Gateway Timeout"`
- Default fallback by range (`2xx` → `"OK"`, `3xx` → `"Redirection"`, `4xx` → `"Client Error"`, `5xx` → `"Server Error"`).
