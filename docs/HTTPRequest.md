# HTTPRequest

## What Problem Does It Solve?

When raw bytes arrive on a socket they are just one long string of text.
Every other class in the server (parser, method handler, CGI, file upload …)
needs to **read specific pieces** of that request — the method, the URI, a
particular header, the body, etc.

`HTTPRequest` is a **data container** (a *model* / *value object*) that gives
every piece of the request its own named field so the rest of the code never
has to re-parse the raw string.

---

## What Does an HTTP Request Look Like?

```
GET /search?q=hello&lang=en HTTP/1.1\r\n     ← request line
Host: example.com\r\n                        ← header
Content-Type: text/html\r\n                  ← header
\r\n                                         ← blank line (end of headers)
<optional body bytes>                        ← body
```

`HTTPRequest` stores each of those sections in its own member variable.

---

## Members Stored

| Field           | Type                              | What it holds                                                                 |
|-----------------|-----------------------------------|-------------------------------------------------------------------------------|
| `method`        | `std::string`                     | `"GET"`, `"POST"`, or `"DELETE"`                                              |
| `uri`           | `std::string`                     | Path **without** the query string, e.g. `"/search"`                           |
| `vers`          | `std::string`                     | `"HTTP/1.0"` or `"HTTP/1.1"`                                                  |
| `headers`       | `std::map<string, string>`        | All headers stored as **lowercase key → value** (e.g. `"host"` → `"example.com"`) |
| `body`          | `std::string`                     | The raw body bytes (may be empty for GET / DELETE)                            |
| `querString`    | `std::string`                     | Everything after `?` in the URI (e.g. `"q=hello&lang=en"`)                   |
| `isComplete`    | `bool`                            | `true` once the parser has received all data for this request                 |
| `isChunked`     | `bool`                            | `true` if `Transfer-Encoding: chunked` was detected                          |
| `contentLength` | `size_t`                          | Value of the `Content-Length` header (0 when absent)                          |

---

## How It Works

1. **Construction** — Every field is initialised to an empty / zero / false
   state via the constructor.
2. **Setters** — `RequestParser` fills the object field-by-field as it walks
   through the raw bytes (`setMethod()`, `setURI()`, `setHeader()`, …).
3. **Getters** — All other components call the const getters to inspect the
   request without touching the internals.
  everyone else just reads it.

> Think of `HTTPRequest` as a **form** (not the HTML kind): the parser fills
> it in, everyone else just reads it.

---

## Relationship With Other Classes

```
Raw bytes ──► RequestParser ──fills──► HTTPRequest
                                           │
                      ┌────────────────────┼────────────────────┐
                      ▼                    ▼                    ▼
               MethodHandler          CGIHandler           FileUpload
              (reads method,        (reads URI,          (reads body,
               URI, headers)        headers, body)        headers)
```
