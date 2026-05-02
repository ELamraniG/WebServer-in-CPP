# Response

## Overview

`Response` is a minimal **Plain Old Data struct** that acts as an intermediate representation of an HTTP response before it is serialized. It is populated by `MethodHandler` (or by `CGIHandler` indirectly) and consumed by `ResponseBuilder::build()` to produce the final HTTP response string.

**Problem it solves:** Rather than having `MethodHandler` construct raw HTTP strings directly, `Response` provides a clean, typed container. This decouples the "what to respond" logic (in `MethodHandler`) from the "how to format it" logic (in `ResponseBuilder`).

---

## Fields

| Field | Type | Default | Purpose |
|---|---|---|---|
| `statusCode` | `int` | `200` | The HTTP status code (e.g., `200`, `404`, `500`) |
| `contentType` | `string` | `"text/html"` | The MIME type for the `Content-Type` header |
| `body` | `string` | `""` | The response body (HTML, file contents, etc.) |
| `headers` | `map<string, string>` | empty | Additional headers to include (e.g., `"Location"`, `"Set-Cookie"`) |

---

## Constructor

```cpp
Response() : statusCode(200), contentType("text/html") {}
```

Defaults to a `200 OK` HTML response with an empty body. All fields are directly accessible and set by `MethodHandler`.

---

## Usage Flow

```
MethodHandler::handleGET()
        │
        ▼
    populates Response { statusCode, contentType, body, headers }
        │
        ▼
ResponseBuilder::build(resp)
        │
        ▼
    "HTTP/1.0 200 OK\r\nDate: ...\r\nContent-Type: text/html\r\n...\r\n<body>"
        │
        ▼
Client::setResponse(rawString)  →  written to socket
```
