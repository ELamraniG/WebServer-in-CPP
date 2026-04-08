# ResponseBuilder

## What Problem Does It Solve?

The `MethodHandler` produces a `Response` struct with a status code, content
type, body, and optional extra headers. But the client expects **raw HTTP
bytes** in a very specific format. Something needs to **serialise** the
`Response` struct into that exact format.

`ResponseBuilder` is that serialiser — it takes a `Response` and produces the
exact string of bytes to send over the socket with `send()`.

---

## What Does a Raw HTTP Response Look Like?

```
HTTP/1.0 200 OK\r\n
Date: Tue, 08 Apr 2026 12:30:00 GMT\r\n
Server: webserv/1.0\r\n
Location: /new-path\r\n                      ← extra header (optional)
Content-Type: text/html\r\n
Content-Length: 45\r\n
Connection: close\r\n
\r\n                                          ← blank line = end of headers
<html><body>Hello World</body></html>         ← body
```

---

## How It Builds the Response — Step by Step

### `build(Response)` — the main method

```
1. STATUS LINE
   "HTTP/1.0 " + statusCode + " " + reasonPhrase + "\r\n"
   e.g. "HTTP/1.0 404 Not Found\r\n"

2. DATE HEADER
   "Date: Tue, 08 Apr 2026 12:30:00 GMT\r\n"
   Generated with gmtime() in RFC 7231 format.

3. SERVER HEADER
   "Server: webserv/1.0\r\n"
   Always included (42 project lets us pick a name).

4. EXTRA HEADERS
   Loop over resp.headers map:
   → Skip "content-type" and "content-length" (handled separately below)
   → Output each as "Key: Value\r\n"
   (e.g. "Location: /new-path\r\n")

5. CONTENT-TYPE and CONTENT-LENGTH
   Skipped entirely for 204 (No Content) and 304 (Not Modified)
   because those responses MUST NOT have a body.
   Otherwise:
   → "Content-Type: text/html\r\n"
   → "Content-Length: 45\r\n"

6. CONNECTION HEADER
   "Connection: close\r\n"
   Always sent — our server closes after each response (HTTP/1.0 style).

7. BLANK LINE
   "\r\n"
   Marks end of headers.

8. BODY
   Appended raw (skipped for 204/304).
```

### `buildError(int code)` — shortcut for error pages

Used by the **server loop** when it needs to reject a request before
`MethodHandler` is even reached (e.g. parser returned `P_ERROR`, client
timed out).

```
1. Creates a Response struct
2. Sets statusCode = code, contentType = "text/html"
3. Generates a simple HTML body:
   "<h1>404 Not Found</h1>"
4. Calls build() on that Response
```

---

## Reason Phrase Mapping

`reasonPhrase(int code)` converts status codes to their standard text:

| Code | Reason Phrase           |
|------|-------------------------|
| 200  | OK                      |
| 201  | Created                 |
| 204  | No Content              |
| 301  | Moved Permanently       |
| 302  | Found                   |
| 400  | Bad Request             |
| 403  | Forbidden               |
| 404  | Not Found               |
| 405  | Method Not Allowed      |
| 413  | Payload Too Large       |
| 500  | Internal Server Error   |
| 502  | Bad Gateway             |
| 505  | HTTP Version Not Supported |

For unrecognised codes, it falls back to a generic phrase based on the code
class (2xx → "OK", 4xx → "Client Error", 5xx → "Server Error").

---

## Special Cases

| Condition         | Behaviour                                                |
|-------------------|----------------------------------------------------------|
| Status 204 or 304 | No `Content-Type`, no `Content-Length`, no body           |
| Extra headers that duplicate `content-type` / `content-length` | Silently skipped (the builder always writes its own) |

---

## Relationship With Other Classes

```
    MethodHandler          Server Loop
         │                      │
         │  returns Response    │  parse error / timeout
         │                      │
         ▼                      ▼
    ┌──────────────────────────────────┐
    │         ResponseBuilder          │
    │                                  │
    │  build(Response) → raw string    │
    │  buildError(code) → raw string   │
    └──────────────────────────────────┘
                   │
                   ▼
           send() to socket
```
