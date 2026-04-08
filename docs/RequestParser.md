# RequestParser

## What Problem Does It Solve?

The server receives data from the network as a **raw string of bytes**.
Before the server can do anything useful (serve a file, run CGI, upload…), it
needs to **break that string apart** into structured data: method, URI,
headers, and body.

`RequestParser` does exactly that — it takes the raw bytes and **populates an
`HTTPRequest` object** with every piece of information, or tells the caller
"I need more data" / "this is invalid".

---

## What Does a Raw HTTP Request Look Like?

An HTTP request is **always** made up of these parts, in this order:

```
┌─────────────────────────────────────────────────┐
│  1. REQUEST LINE  (mandatory, always present)   │
│     METHOD  SP  URI  SP  VERSION  \r\n          │
│                                                 │
│     Example: GET /index.html HTTP/1.1\r\n       │
├─────────────────────────────────────────────────┤
│  2. HEADERS  (zero or more, each ends with \r\n)│
│     Key: Value\r\n                              │
│     Key: Value\r\n                              │
│     ...                                         │
│     \r\n       ← blank line = end of headers    │
├─────────────────────────────────────────────────┤
│  3. BODY  (optional)                            │
│     Raw bytes whose length is determined by:    │
│       • Content-Length header, OR                │
│       • Transfer-Encoding: chunked              │
│     If neither is present → no body.            │
└─────────────────────────────────────────────────┘
```

### The Request Line (Part 1 — always required)

The very first line **must** contain exactly three tokens separated by spaces:

| Token    | Rules                                          | Example                    |
|----------|------------------------------------------------|----------------------------|
| METHOD   | Must be `GET`, `POST`, or `DELETE`             | `GET`                      |
| URI      | The path, optionally followed by `?query`      | `/search?q=hello&lang=en`  |
| VERSION  | Must be `HTTP/1.0` or `HTTP/1.1`               | `HTTP/1.1`                 |

- If there are **fewer than 3 tokens** → parse error.
- If there are **more than 3 tokens** → parse error (no extra words allowed).
- If the **method is not** GET/POST/DELETE → parse error.
- If the **version is not** HTTP/1.0 or HTTP/1.1 → parse error.
- If the URI contains a `?`, the part after `?` is split off and stored as the
  **query string** (e.g. `q=hello&lang=en`). The URI itself keeps only the
  path portion (`/search`).

### The Headers (Part 2 — optional but common)

Headers sit between the request line and the body, separated by `\r\n\r\n`
at the end.

Each header line looks like:

```
Key: Value\r\n
```

- The **key** is everything before the first `:`.
- The **value** is everything after the `:`, with leading/trailing whitespace
  stripped.
- Keys are **lowercased** before storage (HTTP headers are case-insensitive).
- An empty key → parse error.
- A line with no `:` at all → parse error.
- Having **zero headers** is valid (just `\r\n` right after the request line).

### The Body (Part 3 — optional)

The body is everything **after the `\r\n\r\n`** that ends the headers.
Whether there *is* a body, and how to read it, depends on two headers:

| Scenario                              | How body is read                                                     |
|---------------------------------------|----------------------------------------------------------------------|
| `Transfer-Encoding: chunked`          | Body is in **chunked encoding** — delegated to `ChunksDecoding`      |
| `Content-Length: N`                    | Read exactly **N bytes** after the blank line                        |
| Neither header present                | **No body** — request is complete immediately                        |

- If the body is chunked, `ChunksDecoding::decode()` is called. It may return
  `DECODE_NEED_MORE` (we wait for more data), `DECODE_ERROR` (bad format), or
  `DECODE_COMPLETE` (body is ready).
- If `Content-Length` is present but the value is **not a valid number** →
  parse error.
- If `Content-Length` says 100 bytes but only 50 have arrived → `P_INCOMPLETE`
  (the caller will feed more data later).

---

## The Three Return States

`parseRequest()` returns one of:

| Status         | Meaning                                        | What the caller should do              |
|----------------|------------------------------------------------|----------------------------------------|
| `P_SUCCESS`    | Request is fully parsed and valid              | Process the request                    |
| `P_INCOMPLETE` | Not enough data has arrived yet                | Read more from the socket, call again  |
| `P_ERROR`      | The data is malformed / invalid                | Send 400 Bad Request to the client     |

---

## Step-by-Step Walkthrough

Here is the exact sequence inside `parseRequest()`:

```
rawBytes = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
                                                                  
  1.  Find first \r\n  →  position 26                             
      Extract: "GET /index.html HTTP/1.1"                         
      Call parseFirstLine() → splits into GET, /index.html, HTTP/1.1
                                                                  
  2.  Find \r\n\r\n  →  position 44                               
      Extract headers from pos 28..44: "Host: localhost"          
      Call parseHeaders() → stores { "host": "localhost" }        
                                                                  
  3.  bodyStart = 44 + 4 = 48                                     
                                                                  
  4.  Check Transfer-Encoding: chunked?  → no                    
      Check Content-Length?              → no                     
      Neither → no body, request is complete.                     
                                                                  
  5.  Return P_SUCCESS                                            
```

---

## Relationship With Other Classes

```
           ┌──────────────┐
           │ ChunksDecoding│  ← member: handles chunked bodies
           └──────┬───────┘
                  │
  raw bytes ──► RequestParser ──fills──► HTTPRequest
                  │
                  │  returns P_SUCCESS / P_INCOMPLETE / P_ERROR
                  ▼
            Server loop decides what to do next
```

---

## Public Interface

| Method                          | Purpose                                            |
|---------------------------------|----------------------------------------------------|
| `parseRequest(rawBytes, req)`   | Main entry — parses everything, fills `req`         |
| `parseFirstLine(line, req)`     | Parses the `METHOD URI VERSION` line                |
| `parseHeaders(headerBlock, req)`| Parses all `Key: Value` lines into the request map  |
