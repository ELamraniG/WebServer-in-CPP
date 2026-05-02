# RequestParser

## Overview

`RequestParser` converts raw TCP byte data (a `string` of accumulated bytes from `Client::_requestBuffer`) into a fully structured `HTTPRequest` object. It handles all three phases of an HTTP/1.x message: the request line, the headers, and the body (including `Content-Length` and `Transfer-Encoding: chunked`).

**Problem it solves:** HTTP data arrives incrementally over the network — a single `read()` call may only get part of the headers, or the body may span many packets. `RequestParser` must handle partial data gracefully by returning `P_INCOMPLETE` rather than failing, and must resume correctly when called again with more data.

---

## Enum: `ParsingStatus`

| Value | Meaning |
|---|---|
| `P_SUCCESS` | The complete request has been parsed successfully |
| `P_INCOMPLETE` | Not enough data yet; caller should wait for more and call again |
| `P_ERROR` | The request is malformed; caller should return `400 Bad Request` |

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `chunksDecoding` | `ChunksDecoding` | Handles chunked transfer-encoding body decoding |

---

## Public Functions

### `parseRequest(const string& rawBytes, HTTPRequest& request)` → `ParsingStatus`
**Purpose:** Top-level entry point. Called by `EventLoop` every time new data arrives on a client socket.

**Problem:** The raw buffer may contain only part of the headers, part of the body, or multiple complete requests in edge cases. Each call must re-examine the full buffer from the beginning and determine if the request is complete, incomplete, or malformed.

**How it works:**
1. **Find the first line:** Searches for `\r\n`. Returns `P_INCOMPLETE` if not found yet.
2. **Parse the request line:** Calls `parseFirstLine(requestLine, request)`. Returns `P_ERROR` if malformed.
3. **Find the header block end:** Searches for `\r\n\r\n`. Returns `P_INCOMPLETE` if not found.
4. **Parse headers:** Extracts the substring between the first `\r\n` and the `\r\n\r\n`, calls `parseHeaders(theHeader, request)`. Returns `P_ERROR` if malformed.
5. **Handle body:**
   - **Chunked:** If `Transfer-Encoding: chunked`, delegates to `chunksDecoding.decode()`. Returns `P_INCOMPLETE` or `P_ERROR` accordingly. On `DECODE_COMPLETE`, sets the decoded body on the request.
   - **Content-Length:** Reads the `Content-Length` header value. If the buffer doesn't yet contain that many bytes after the headers, returns `P_INCOMPLETE`. Otherwise sets the body.
   - **No body:** Sets `contentLength = 0`, marks the request as complete.
6. Returns `P_SUCCESS` when the full request is assembled.

---

### `parseFirstLine(const string& one_line, HTTPRequest& request)` → `bool`
**Purpose:** Parses `"GET /path?query HTTP/1.1"` into method, URI, and version.

**Problem:** The first line must have exactly three tokens. The URI may contain a query string after `?` and may be percent-encoded. The HTTP version must be `HTTP/1.0` or `HTTP/1.1`.

**How it works:**
1. Uses `std::istringstream` to split the line into `method`, `uri`, `version`.
2. Checks that no extra tokens exist after the three expected ones — returns `false` if so.
3. Rejects any version other than `HTTP/1.0` or `HTTP/1.1`.
4. If `?` is found in the URI: splits it, stores everything after `?` as `queryString`, trims the URI to just the path.
5. Calls `urlDecode(uri)` to percent-decode the path (e.g., `%20` → space).
6. Sets method, URI, and version on the `HTTPRequest` object.

---

### `parseHeaders(const string& theHeader, HTTPRequest& request)` → `bool`
**Purpose:** Parses all HTTP headers from the multi-line block between the request line and `\r\n\r\n`.

**Problem:** Headers are line-delimited (`\r\n`). Each line is `Key: Value`. The key is case-insensitive. Empty lines within the block should be skipped. A line missing `:` is an error.

**How it works:**
1. Iterates through `theHeader` line by line (splitting on `\r\n`).
2. Skips empty lines.
3. For each non-empty line: finds the first `:`, splits into `key` and `value`.
4. Returns `false` if no `:` is found or the key is empty.
5. Calls `strTrim()` on both key and value to remove whitespace.
6. Stores via `request.setHeader(strToLower(key), value)` — the lowercase key triggers automatic cookie parsing if the key is `"cookie"`.

---

### `urlDecode(const string& str)` — Static internal helper
**Purpose:** Decodes percent-encoded URI characters (e.g., `%20` → ` `, `%2F` → `/`).

**How it works:**
Iterates character by character. When `%` is encountered followed by exactly two hex digits, converts the hex pair to a character and advances the index by 2. All other characters are passed through unchanged.
