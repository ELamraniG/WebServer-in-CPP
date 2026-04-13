# MethodHandler

## What Problem Does It Solve?

Once the `RequestParser` has turned raw bytes into a structured `HTTPRequest`,
the server needs to **actually do something** with it — serve a file, handle
a file upload, delete a resource, or run a CGI script.

`MethodHandler` is the **dispatcher and executor** for the three HTTP methods
the server supports: **GET**, **POST**, and **DELETE**. Given a request and
the route configuration, it figures out what action to take and returns a
`Response`.

---

## Overview of Each Method

### GET — "Give me this resource"

The GET handler follows this decision tree:

```
GET request arrives
│
├─ Is the method allowed by the route config? ─── No → 405 Method Not Allowed
├─ Does the URI contain path traversal (..)?  ─── Yes → 403 Forbidden
├─ Is there a redirect configured?            ─── Yes → 301 redirect
├─ Is this a CGI request?                     ─── Yes → delegate to CGIHandler
│
├─ Resolve file path = root + URI
│
├─ Is it a directory?
│   ├─ URI doesn't end with /             → 301 redirect to URI + "/"
│   ├─ Index file configured & exists?    → serve the index file
│   ├─ Autoindex enabled?                → generate directory listing HTML
│   └─ None of the above                 → 403 Forbidden
│
├─ File doesn't exist?                   → 404 Not Found
├─ File not readable?                    → 403 Forbidden
│
└─ Serve the file
   ├─ Read content from disk
   ├─ Detect MIME type from extension
   └─ Return 200 with body
```

### POST — "Process this data"

```
POST request arrives
│
├─ Is the method allowed?                    → No → 405
├─ Path traversal?                           → Yes → 403
├─ Redirect configured?                     → Yes → 301
├─ Body too large (exceeds maxBodySize)?     → Yes → 413 Payload Too Large
├─ Is this a CGI request?                   → Yes → delegate to CGIHandler
│
├─ Is Content-Type multipart/form-data?
│   ├─ Yes → file upload flow:
│   │   ├─ Parse multipart body → extract one file (FileUpload)
│   │   ├─ Parsing failure → 400 Bad Request
│   │   ├─ Save the file to uploadStore
│   │   └─ Return 201 Created (+ Location) or 500 on save failure
│   │
│   └─ No → generic body:
│       └─ Return 200 OK with acknowledgement
```

### DELETE — "Remove this resource"

```
DELETE request arrives
│
├─ Is the method allowed?     → No → 405
├─ Path traversal?            → Yes → 403
├─ Is target a directory?     → Yes → 403 (cannot delete dirs)
├─ File doesn't exist?        → 404
├─ No write permission?       → 403
│
└─ Remove the file with std::remove()
   ├─ Success → 204 No Content
   └─ Failure → 500 Internal Server Error
```

---

## Security Checks (Common to All Methods)

Before any method does its work, two checks run:

1. **Method allowed?** — The `RouteConfig` has a list of allowed methods. If
   the current method isn't in the list (and the list isn't empty), return 405.
2. **Path traversal?** — The URI is scanned for `..` segments and `\`
   characters. If found → 403 Forbidden. This prevents clients from escaping
   the document root (e.g. `GET /../../etc/passwd`).

---

## MIME Type Detection

`getTheFileType()` lowercases extensions and maps them to Content-Type values.
It includes:
- text: `.html`, `.htm`, `.css`, `.txt`, `.csv`, `.xml`
- script/data: `.js`, `.json`, `.pdf`, `.zip`
- images: `.jpg`, `.jpeg`, `.png`, `.gif`, `.bmp`, `.svg`, `.ico`, `.webp`
- media: `.mp3`, `.mp4`, `.webm`
- fonts: `.woff`, `.woff2`, `.ttf`

Unknown or missing extension falls back to `application/octet-stream`.

The extension must appear after the final `/` in the path to be considered valid.

---

## Directory Listing (Autoindex)

When a GET request targets a directory and `autoindex` is enabled in the route
config, the handler generates an HTML page listing all files and
subdirectories, sorted alphabetically. Subdirectories display a trailing `/`.

---

## Helper Functions

| Function         | Purpose                                                         |
|------------------|-----------------------------------------------------------------|
| `isAllowed()`    | Check if the method is in the route's allowed list              |
| `RemoveQueryString()` | Remove `?query` from URI before filesystem operations     |
| `isItUnsafe()`   | Detect `..` traversal or `\` in the URI                         |
| `makeThePath()`  | Combine document root + URI into an absolute filesystem path    |
| `readFileContent()` | Read entire file into a string (binary mode)                 |
| `isDirectory()`  | Check if a path is a directory (using `stat`)                   |
| `fileExists()`   | Check if a path exists on disk                                  |
| `isReadable()`   | Check if we have read permission (`access(R_OK)`)               |
| `canDeletePath()`| Check write + execute permission on the parent directory        |
| `makeError()`    | Build a minimal HTML error `Response` with given code           |
| `makeRedirect()` | Build a 301 redirect `Response` with `Location` header          |
| `tryCGI()`       | Dispatch to `CGIHandler` if the URI matches a CGI extension     |
| `buildAutoindex()`| Generate the HTML directory listing                            |
| `ensureSession()`| Create/reuse simple `SESSION_ID` cookie-backed session          |
| `applySession()` | Apply session creation logic to every outgoing response          |

## Simple Session Example

First request (no cookie):

```
GET /index.html HTTP/1.1
Host: localhost

```

Response contains a new session cookie:

```
Set-Cookie: SESSION_ID=abc123xyz
```

Next request (cookie sent back):

```
GET /index.html HTTP/1.1
Host: localhost
Cookie: SESSION_ID=abc123xyz

```

Server reuses the existing session and does not need to issue a new cookie.

---

## Relationship With Other Classes

```
                   HTTPRequest
                       │
                       ▼
                 MethodHandler
                  /    |    \
                 /     |     \
           GET  /  POST|  DELETE\
               /       |        \
              ▼        ▼         ▼
         serve file  FileUpload  std::remove()
         autoindex   CGIHandler
         CGIHandler
              \       |        /
               \      |       /
                ▼     ▼      ▼
                  Response
                     │
                     ▼
               ResponseBuilder
```
