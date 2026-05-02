# HTTPRequest

## Overview

`HTTPRequest` is a **data class** that holds all components of a parsed HTTP request in a structured, typed form. It is populated by `RequestParser` and then read by `MethodHandler`, `Router`, `CGIHandler`, and `EventLoop` to make decisions about how to serve the request.

**Problem it solves:** Raw HTTP data arrives as a stream of bytes (`"GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n"`). `HTTPRequest` provides a clean, structured interface with typed getters and setters so that every other component can simply call `request.getMethod()` or `request.getHeader("host")` instead of parsing raw strings.

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `method` | `string` | The HTTP method (`"GET"`, `"POST"`, `"DELETE"`) |
| `uri` | `string` | The request path, URL-decoded, without the query string (e.g., `"/pages/index.html"`) |
| `vers` | `string` | The HTTP version (`"HTTP/1.0"` or `"HTTP/1.1"`) |
| `headers` | `map<string, string>` | All HTTP headers, keys lowercased (e.g., `{"host": "localhost", "content-type": "..."}`) |
| `cookies` | `map<string, string>` | Parsed cookies from the `Cookie` header (e.g., `{"SESSION_ID": "abc123"}`) |
| `body` | `string` | The request body (decoded if chunked) |
| `queryString` | `string` | The raw query string after `?` (e.g., `"id=1&name=test"`) |
| `isComplete` | `bool` | `true` when the full request (headers + body) has been received |
| `isChunked` | `bool` | `true` if `Transfer-Encoding: chunked` was present |
| `contentLength` | `size_t` | Value of the `Content-Length` header |
| `isCGI` | `bool` | Set to `true` by `MethodHandler` when the request maps to a CGI script |

---

## Public Functions

### Getters

| Function | Returns |
|---|---|
| `getMethod()` | The HTTP method string |
| `getURI()` | The decoded URI path |
| `getVersion()` | The HTTP version string |
| `getQueryString()` | The query string (everything after `?`) |
| `getBody()` | The request body |
| `getHeader(key)` | The value of a specific header (empty string if not present); key is case-insensitive since all keys are stored lowercase |
| `getCookie(key)` | The value of a specific cookie |
| `getAllHeaders()` | A copy of the entire headers map |
| `isItCompleted()` | `true` if the full request has been received |
| `isItChunked()` | `true` if the body used chunked transfer encoding |
| `getIsCGI()` | `true` if this request should be handled by CGI |
| `getContentLength()` | The content length value |

---

### Setters

Most setters are straightforward assignments. The one with special behavior is:

#### `setHeader(const string& key, const string& value)`
**Purpose:** Stores a header in the `headers` map. Additionally, if the key is `"cookie"`, it automatically parses the cookie string into the `cookies` map.

**How it works:**
1. Stores `headers[key] = value`.
2. If `key == "cookie"`: calls the internal `parseCookiesFromHeader(value, cookies)` function.

#### `parseCookiesFromHeader(value, cookies)` — Static internal helper
**Problem:** The `Cookie` header holds multiple cookies in one string: `"SESSION_ID=abc; user=john"`. Each must be split into individual key-value pairs.

**How it works:**
1. Splits the cookie string on `;` separators.
2. For each part, finds the first `=` and splits into key/value.
3. Trims whitespace from both key and value.
4. Stores non-empty key-value pairs in the `cookies` map.
