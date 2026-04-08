# Response (struct)

## What Problem Does It Solve?

When the server has finished processing a request (serving a file, running
CGI, uploading, deleting …), it needs to **carry the result** from the
`MethodHandler` to the `ResponseBuilder` which will turn it into raw bytes
for the wire.

`Response` is a **simple data struct** — a container that holds all the
pieces needed to build a valid HTTP response.

---

## What Does an HTTP Response Look Like?

```
HTTP/1.0 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 45\r\n
Location: /new-path\r\n        ← extra header (only sometimes)
\r\n
<html><body>Hello</body></html>  ← body
```

`Response` stores the parts that **vary from one response to another**:

---

## Members

| Field         | Type                       | What it holds                                     | Default    |
|---------------|----------------------------|---------------------------------------------------|------------|
| `statusCode`  | `int`                      | HTTP status code (200, 404, 500, …)               | `200`      |
| `contentType` | `std::string`              | MIME type of the body (`text/html`, `image/png`…) | `"text/html"` |
| `body`        | `std::string`              | The actual content to send to the client          | `""`       |
| `headers`     | `std::map<string, string>` | Extra headers (`Location`, `Set-Cookie`, etc.)    | empty map  |

---

## How It's Used

```
 MethodHandler                      ResponseBuilder
      │                                   │
      │  1. Creates a Response            │
      │  2. Sets statusCode, body, etc.   │
      │  3. Returns it                    │
      │                                   │
      └──────────► Response ─────────────►│
                                          │
                                    4. Serialises into
                                       raw HTTP bytes
                                          │
                                          ▼
                                   "HTTP/1.0 200 OK\r\n..."
```

1. `MethodHandler` (or `CGIHandler`) fills a `Response` struct.
2. The server loop passes it to `ResponseBuilder::build()`.
3. `ResponseBuilder` reads the struct and produces the final string that goes
   on the wire via `send()`.

---

## Why a Struct and Not a Class?

`Response` has **no logic** — no validation, no transformation. It's pure
data in → data out. Making it a struct with public members keeps things
simple: anyone can set any field directly, and `ResponseBuilder` just reads
them.
