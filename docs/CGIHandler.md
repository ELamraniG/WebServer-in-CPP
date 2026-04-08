# CGIHandler

## What Problem Does It Solve?

Sometimes the server can't just serve a static file — it needs to **run a
program** to generate the response dynamically. For example, a PHP script
that queries a database, or a Python script that processes form data.

**CGI (Common Gateway Interface)** is the standard protocol for this: the
server launches an external program, passes it information about the request
via environment variables and stdin, and reads the program's output (stdout)
as the response.

`CGIHandler` manages this entire lifecycle:
1. **Detect** whether a request should be handled by CGI.
2. **Execute** the CGI script as a child process.
3. **Collect** the script's output and return it as a `CGIResult`.

---

## How CGI Works

```
┌──────────┐         ┌─────────────┐         ┌──────────┐
│  Client   │ ──────► │   Server    │ ──────► │ CGI Script│
│           │         │ (webserv)   │         │ (e.g. PHP) │
│           │ ◄────── │             │ ◄────── │           │
└──────────┘         └─────────────┘         └──────────┘
                          │                       ▲
                          │                       │
                     fork + exec            reads env vars
                     pipe stdin ──────────► stdin
                     pipe stdout ◄──────── stdout
```

### Step 1: Is this a CGI request?

`isCGIRequest()` checks whether the URI matches a CGI extension configured
in the route (e.g. `.php`, `.py`, `.cgi`) and whether a CGI interpreter is
configured (`cgiPass` in `RouteConfig`).

### Step 2: Build environment variables

CGI scripts receive request information via **environment variables**, not
function parameters. The server must set:

| Variable            | Value                              | Example                     |
|---------------------|------------------------------------|-----------------------------|
| `REQUEST_METHOD`    | The HTTP method                    | `GET`                       |
| `QUERY_STRING`      | Query part of the URI              | `q=hello&lang=en`           |
| `CONTENT_TYPE`      | Content-Type header value          | `application/json`          |
| `CONTENT_LENGTH`    | Body size in bytes                 | `42`                        |
| `SCRIPT_FILENAME`   | Full path to the script on disk    | `/var/www/cgi/script.php`   |
| `PATH_INFO`         | Extra path after the script name   | `/extra/path`               |
| `SERVER_PROTOCOL`   | HTTP version                       | `HTTP/1.1`                  |
| `SERVER_PORT`       | Port the server listens on         | `8080`                      |
| `HTTP_*`            | All request headers                | `HTTP_HOST=localhost`       |

`buildEnvironment()` constructs a `char**` array (null-terminated) from the
request, following the CGI specification.

### Step 3: Execute the script

```
1. Create two pipes:
   - stdin pipe  → server writes request body to it
   - stdout pipe → server reads script output from it

2. fork()
   - Child process:
     ├─ Redirect stdin to the input pipe
     ├─ Redirect stdout to the output pipe
     ├─ Set environment variables
     └─ execve(cgiPass, [cgiPass, scriptPath], envp)
   
   - Parent process:
     ├─ Write request body to stdin pipe
     ├─ Close stdin pipe (signals EOF to script)
     ├─ Read all output from stdout pipe
     └─ waitpid() for child to finish
```

### Step 4: Parse CGI output

The CGI script outputs **headers followed by a body**, similar to HTTP:

```
Content-Type: text/html\r\n
Status: 200\r\n
\r\n
<html>Dynamic content here</html>
```

The handler parses these CGI headers to extract:
- `Status` → becomes the HTTP status code
- `Content-Type` → becomes the response content type
- Other headers → passed through to the client

---

## The CGIResult Struct

```cpp
struct CGIResult {
    bool success;            // did the script run successfully?
    int statusCode;          // HTTP status code (200, 500, etc.)
    std::map<std::string, std::string> headers;  // CGI output headers
    std::string body;        // CGI output body
    std::string errorMessage; // error description if success == false
};
```

---

## Error Scenarios

| Situation                        | Result                                      |
|----------------------------------|---------------------------------------------|
| CGI interpreter not found        | `success = false`, 500 error                |
| Script file doesn't exist        | `success = false`, 404 error                |
| Script has no execute permission | `success = false`, 403 error                |
| Script crashes / non-zero exit   | `success = false`, 502 Bad Gateway          |
| Script times out                 | `success = false`, 504 Gateway Timeout      |
| pipe() or fork() fails           | `success = false`, 500 error                |

---

## Relationship With Other Classes

```
    MethodHandler (GET or POST)
         │
         │  calls isCGIRequest()
         │  if true → calls execute()
         │
         ▼
    CGIHandler
    ├── isCGIRequest(uri, route)
    │   └── checks if extension matches + cgiPass is set
    │
    ├── execute(request, route)
    │   ├── buildScriptPath()
    │   ├── buildEnvironment()
    │   ├── fork() + execve()
    │   ├── pipe stdin (body) → script
    │   ├── pipe stdout ← script
    │   └── return CGIResult
    │
    └── freeEnvironment()   ← cleanup char** array

    CGIResult → converted to Response by MethodHandler
```
