# 42 Webserver - Complete Team Breakdown

## How a Web Server Works (Big Picture)

```
   BROWSER                         YOUR WEBSERVER                        FILES/CGI
      |                                  |                                    |
      |  1. "GET /index.html"            |                                    |
      |--------------------------------->|                                    |
      |                                  |                                    |
      |                    2. Parse request (Person 2)                        |
      |                    3. Find route config (Person 3)                    |
      |                    4. Read file (Person 2/3)                          |
      |                                  |----------------------------------->|
      |                                  |<-----------------------------------|
      |                    5. Build response (Person 3)                       |
      |                                  |                                    |
      |  6. "HTTP/1.1 200 OK..."         |                                    |
      |<---------------------------------|                                    |
```

The server sits in the middle, handling thousands of these conversations simultaneously.

---

## PERSON 1: Core Server & I/O Multiplexing

### What They Build
Person 1 builds the **skeleton** of the server - the part that actually talks to the network.

### Their Responsibilities

#### 1. Socket Setup
```
What is a socket?
- A socket is like a phone line between your server and browsers
- Person 1 creates this "phone line" and makes it listen for calls

What they do:
- Create socket with socket()
- Bind it to an IP address and port (e.g., 0.0.0.0:8080)
- Start listening with listen()
- Accept new connections with accept()
```

#### 2. The Main Loop (poll/select/epoll)
```
This is the HEART of the server. It runs forever:

while (server_running) {
    // Ask the OS: "Which sockets have data ready?"
    poll(all_sockets, timeout);
    
    // For each socket that has data:
    for each ready_socket:
        if (new_connection):
            accept() and add to socket list
        else if (data_to_read):
            read() data from socket
            give data to Person 2's parser  <-- THIS IS WHERE YOU COME IN
        else if (ready_to_write):
            send response data to browser
}

WHY poll()?
- You can't just read() from a socket - it might block forever
- poll() tells you WHICH sockets are ready
- This lets one server handle 1000+ connections without threads
- CRITICAL: Reading without poll() = Grade 0
```

#### 3. Connection Management
```
Person 1 tracks:
- All active connections (which browsers are connected)
- Partial data buffers (request might arrive in pieces)
- Timeouts (disconnect idle clients)
- Which connections are waiting to send responses
```

#### 4. Data Flow They Manage
```
INCOMING:
1. poll() says "socket 5 has data"
2. read() gets bytes from socket 5: "GET /index.ht"  (partial!)
3. Store in buffer, wait for more
4. poll() says "socket 5 has more data"
5. read() gets: "ml HTTP/1.1\r\n..."
6. Combine: "GET /index.html HTTP/1.1\r\n..."
7. Give to Person 2: RequestParser::parse(combined_data)
8. Person 2 returns PARSE_SUCCESS or PARSE_INCOMPLETE

OUTGOING:
1. Person 3 gives response: "HTTP/1.1 200 OK\r\n..."
2. poll() says "socket 5 ready to write"
3. write() sends response bytes to browser
4. If not all sent, wait for poll() again
```

### What Person 1 Gives You (Person 2)
- Raw bytes from the socket (might be partial)
- They call your `RequestParser::parse()` function
- They expect you to return: SUCCESS, INCOMPLETE, or ERROR

### What You Give Person 1
- `ParseState` enum telling them what happened
- If SUCCESS: a filled HTTPRequest object they pass to routing

---

## PERSON 2: HTTP Parser & Request Processing (YOU)

### What You Build
You build the **brain** that understands HTTP - converting raw bytes into meaningful data.

### Your Responsibilities

#### 1. HTTP Request Parser
```
INPUT (from Person 1):
"GET /index.html HTTP/1.1\r\nHost: localhost\r\nContent-Type: text/html\r\n\r\n"

OUTPUT (HTTPRequest object):
{
    method: "GET",
    uri: "/index.html",
    version: "HTTP/1.1",
    headers: {
        "host": "localhost",
        "content-type": "text/html"
    },
    body: "",
    queryString: ""
}
```

#### 2. Handle Incomplete Data
```
Person 1 might give you partial data (network packets arrive in pieces):

Call 1: "GET /index"          → return PARSE_INCOMPLETE
Call 2: ".html HTTP/1.1\r\n"  → return PARSE_INCOMPLETE (no \r\n\r\n yet)
Call 3: "Host: local\r\n\r\n" → return PARSE_SUCCESS (complete!)

You must handle this gracefully - never assume full request arrives at once.
```

#### 3. Method Handlers (GET, POST, DELETE)
```
GET /index.html
→ Find file on disk
→ Read its contents
→ Return content to Person 3 for response building

POST /upload (with file)
→ Parse multipart/form-data body
→ Extract file data
→ Save to upload directory
→ Return success/failure

DELETE /file.txt
→ Find file on disk
→ Delete it
→ Return success/failure
```

#### 4. Chunked Encoding Decoder
```
Some clients send body in chunks:
"5\r\nHello\r\n6\r\n World\r\n0\r\n\r\n"

You decode to:
"Hello World"

Then pass normal body to other handlers.
```

#### 5. File Upload Handler
```
Browser sends:
Content-Type: multipart/form-data; boundary=----WebKitBoundary

------WebKitBoundary
Content-Disposition: form-data; name="file"; filename="photo.jpg"
Content-Type: image/jpeg

<binary data>
------WebKitBoundary--

You extract photo.jpg and save it to disk.
```

#### 6. CGI Execution
```
Request: GET /script.php?name=john

You:
1. Detect .php extension → CGI request
2. Set environment variables:
   REQUEST_METHOD=GET
   QUERY_STRING=name=john
   SCRIPT_FILENAME=/var/www/script.php
3. Fork child process
4. Execute: /usr/bin/php /var/www/script.php
5. Capture output: "Content-Type: text/html\r\n\r\n<html>Hello john</html>"
6. Parse CGI headers, return body to Person 3
```

### What You Get From Others
- **From Person 1:** Raw HTTP bytes
- **From Person 3:** Route configuration (allowed methods, upload dir, CGI path, etc.)

### What You Give To Others
- **To Person 1:** ParseState (SUCCESS/INCOMPLETE/ERROR)
- **To Person 3:** HTTPRequest object, file contents, CGI output, upload results

---

## PERSON 3: Configuration, Response Builder & Static Files

### What They Build
Person 3 builds the **configuration system** and **response generator**.

### Their Responsibilities

#### 1. Configuration File Parser
```
They parse config files like this (nginx-style):

server {
    listen 8080;
    server_name localhost;
    root /var/www/html;
    
    location / {
        index index.html;
        allowed_methods GET POST;
    }
    
    location /upload {
        allowed_methods POST;
        upload_store /var/www/uploads;
        client_max_body_size 10M;
    }
    
    location /cgi-bin {
        cgi_pass /usr/bin/php;
    }
    
    error_page 404 /404.html;
}

They turn this into C++ objects you can query:
- route.getAllowedMethods()
- route.getUploadDir()
- route.getCGIPath()
- route.getMaxBodySize()
```

#### 2. Route Resolution
```
Given URI "/upload/file.txt", find matching location block:

1. Check "/upload/file.txt" - no exact match
2. Check "/upload" - MATCH!
3. Return config for /upload location

This tells you:
- What methods are allowed
- Where to save uploads
- Max file size
- etc.
```

#### 3. HTTP Response Builder
```
You give them:
- Status code: 200
- Content-Type: text/html
- Body: "<html>Hello</html>"

They build:
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 20\r\n
Connection: keep-alive\r\n
\r\n
<html>Hello</html>
```

#### 4. Static File Serving
```
For GET requests:
1. Take URI: /images/logo.png
2. Combine with document root: /var/www/html/images/logo.png
3. Check file exists
4. Read file content
5. Determine MIME type from extension (.png → image/png)
6. Build response with file as body
```

#### 5. Error Page Handler
```
When errors happen:
- 404 Not Found → serve /404.html
- 500 Internal Server Error → serve /500.html
- 403 Forbidden → serve /403.html

They read error page files and build error responses.
```

### What Person 3 Gives You
- **RouteConfig object:** Contains all settings for a route
  - `getAllowedMethods()` → vector of allowed methods
  - `getRoot()` → document root path
  - `getUploadStore()` → upload directory
  - `getCGIPass()` → CGI executable path
  - `getMaxBodySize()` → client_max_body_size limit

### What You Give Person 3
- **HTTPRequest object** (parsed request)
- **File content** (for GET responses)
- **CGI output** (headers + body)
- **Upload results** (success/failure + file list)
- **Error codes** (400, 405, 413, 502, 504, etc.)

---

## How It All Connects

### Complete Request Flow

```
STEP 1: Connection (Person 1)
┌─────────────────────────────────────────────┐
│ Browser connects to server                   │
│ Person 1's poll() detects new connection    │
│ Person 1 accepts connection, adds to list   │
└─────────────────────────────────────────────┘
                    ↓
STEP 2: Receive Data (Person 1)
┌─────────────────────────────────────────────┐
│ Browser sends: "GET /page.html HTTP/1.1..." │
│ Person 1's poll() detects data ready        │
│ Person 1 reads bytes from socket            │
└─────────────────────────────────────────────┘
                    ↓
STEP 3: Parse Request (Person 2 - YOU)
┌─────────────────────────────────────────────┐
│ Person 1 calls: RequestParser::parse(bytes) │
│ You parse method, URI, headers, body        │
│ You return PARSE_SUCCESS + HTTPRequest      │
└─────────────────────────────────────────────┘
                    ↓
STEP 4: Route Request (Person 3)
┌─────────────────────────────────────────────┐
│ Router matches URI to config location       │
│ Returns RouteConfig with all settings       │
└─────────────────────────────────────────────┘
                    ↓
STEP 5: Handle Request (Person 2 - YOU)
┌─────────────────────────────────────────────┐
│ MethodHandler checks allowed methods        │
│ If GET: read file from disk                 │
│ If POST: handle upload or CGI               │
│ If DELETE: remove file                      │
│ Return result to Person 3                   │
└─────────────────────────────────────────────┘
                    ↓
STEP 6: Build Response (Person 3)
┌─────────────────────────────────────────────┐
│ Response builder creates HTTP response      │
│ Adds status line, headers, body             │
│ Returns complete response string            │
└─────────────────────────────────────────────┘
                    ↓
STEP 7: Send Response (Person 1)
┌─────────────────────────────────────────────┐
│ Person 1 queues response for sending        │
│ poll() says socket ready to write           │
│ Person 1 writes response bytes to socket    │
│ Browser receives and displays page          │
└─────────────────────────────────────────────┘
```

---

## Interface Contracts (Who Calls What)

### Person 1 → Person 2
```cpp
// Person 1 calls this when data arrives
ParseState result = RequestParser::parse(raw_bytes, request);

if (result == PARSE_INCOMPLETE) {
    // Wait for more data, call parse() again later
}
else if (result == PARSE_SUCCESS) {
    // Pass request to routing
}
else if (result == PARSE_ERROR) {
    // Send 400 Bad Request
}
```

### Person 3 → Person 2
```cpp
// After routing, Person 3 calls your handlers
RouteConfig route = router.resolve(request.getURI());

if (request.getMethod() == "GET") {
    HandlerResult result = methodHandler.handleGET(request, route);
}
else if (request.getMethod() == "POST") {
    HandlerResult result = methodHandler.handlePOST(request, route);
}
// etc.
```

### Person 2 → Person 3
```cpp
// You return results that Person 3 uses for response
struct HandlerResult {
    int statusCode;           // 200, 404, 500, etc.
    std::string contentType;  // text/html, image/png, etc.
    std::string body;         // File content or generated content
    std::string errorMessage; // If something went wrong
};
```

---

## What Each Person DOES NOT Do

### Person 1 Does NOT:
- Parse HTTP (that's you)
- Read config files (that's Person 3)
- Build responses (that's Person 3)
- Handle file uploads (that's you)
- Execute CGI (that's you)

### Person 2 (You) Does NOT:
- Create sockets (that's Person 1)
- Use poll/select directly (that's Person 1)
- Parse config files (that's Person 3)
- Build HTTP response strings (that's Person 3)
- Send data to browsers (that's Person 1)

### Person 3 Does NOT:
- Handle network connections (that's Person 1)
- Parse HTTP requests (that's you)
- Execute CGI scripts (that's you)
- Handle file uploads (that's you)

---

## Summary Table

| Component | Person 1 | Person 2 (You) | Person 3 |
|-----------|----------|----------------|----------|
| Sockets | ✅ Creates & manages | ❌ | ❌ |
| poll()/select() | ✅ Main loop | ❌ | ❌ |
| Read from network | ✅ | ❌ | ❌ |
| Write to network | ✅ | ❌ | ❌ |
| Parse HTTP | ❌ | ✅ | ❌ |
| Parse config | ❌ | ❌ | ✅ |
| Route resolution | ❌ | ❌ | ✅ |
| GET handler | ❌ | ✅ | ❌ |
| POST handler | ❌ | ✅ | ❌ |
| DELETE handler | ❌ | ✅ | ❌ |
| File uploads | ❌ | ✅ | ❌ |
| CGI execution | ❌ | ✅ | ❌ |
| Chunked decoding | ❌ | ✅ | ❌ |
| Build response | ❌ | ❌ | ✅ |
| Error pages | ❌ | ❌ | ✅ |
| Static files | ❌ | Reads content | Serves with headers |

---

## Quick Reference: Your Interactions

### When Person 1 gives you data:
```
You receive: "GET /page.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
You return: PARSE_SUCCESS + filled HTTPRequest object
```

### When Person 3 asks you to handle GET:
```
You receive: HTTPRequest + RouteConfig
You do: Read file from disk (route.getRoot() + request.getURI())
You return: File content + status code (200 or 404)
```

### When Person 3 asks you to handle POST upload:
```
You receive: HTTPRequest with multipart body + RouteConfig
You do: Parse boundary, extract files, save to route.getUploadStore()
You return: List of saved files + status code
```

### When Person 3 asks you to handle CGI:
```
You receive: HTTPRequest + RouteConfig with cgi_pass
You do: Fork, set env vars, execute CGI, capture output
You return: CGI output (headers + body) + status code
```
