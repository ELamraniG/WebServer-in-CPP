# ServerConstants

## Overview

`ServerConstants.hpp` centralizes all compile-time numeric constants and the `HttpStatus` enum used across the server. Nothing here contains logic — it is a shared header included by most other components.

---

## Constants

| Constant | Value | Purpose |
|---|---|---|
| `BUFFER_SIZE` | `4096` | Read buffer size (bytes) for each `readFromSocket()` call |
| `PAUSE` | `0` | Poll event mask used to temporarily stop watching an fd (neither read nor write) — applied to a client fd while its CGI is running |
| `POLL_TIMEOUT` | `5000` | Milliseconds passed to `poll()` as its timeout. Controls how often the event loop wakes up to check timeouts even when no fd is active |
| `CGI_TIMEOUT` | `3` | Seconds before a CGI process is killed and `504 Gateway Timeout` is returned |
| `TIMEOUT` | `45` | Seconds of client inactivity before the connection is closed with `408 Request Timeout` |

---

## HttpStatus Enum

Maps HTTP status codes to named constants. Used throughout the server instead of raw integers to make intent clear.

| Enumerator | Code | Meaning |
|---|---|---|
| `HTTP_CLIENT_DISCONNECTED` | `0` | Internal sentinel — client closed the connection cleanly (not a real HTTP status) |
| `HTTP_OK` | `200` | Successful request |
| `HTTP_CREATED` | `201` | Resource created (POST success) |
| `HTTP_NO_CONTENT` | `204` | Success with no response body (DELETE success) |
| `HTTP_MOVED_PERMANENTLY` | `301` | Permanent redirect |
| `HTTP_FOUND` | `302` | Temporary redirect |
| `HTTP_BAD_REQUEST` | `400` | Malformed request |
| `HTTP_FORBIDDEN` | `403` | Access denied |
| `HTTP_NOT_FOUND` | `404` | Resource not found |
| `HTTP_METHOD_NOT_ALLOWED` | `405` | HTTP method not permitted for this route |
| `HTTP_REQUEST_TIMEOUT` | `408` | Client was idle for longer than `TIMEOUT` seconds |
| `HTTP_CONTENT_TOO_LARGE` | `413` | Request body exceeds the configured `client_max_body_size` |
| `HTTP_INTERNAL_SERVER_ERROR` | `500` | Unrecoverable server-side error |
| `HTTP_NOT_IMPLEMENTED` | `501` | HTTP method is valid but not supported by this server |
| `HTTP_BAD_GATEWAY` | `502` | CGI process returned a malformed response |
| `HTTP_GATEWAY_TIMEOUT` | `504` | CGI process exceeded `CGI_TIMEOUT` seconds |