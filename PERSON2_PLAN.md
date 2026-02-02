# Person 2 - HTTP Parser & Request Processing Implementation Plan

## Overview
As Person 2, I am responsible for the HTTP request parsing, method handling, file uploads, and CGI integration for the 42 webserv project in C++98.

---

## Files to Create

### Directory Structure
```
includes/
    http/
        HTTPRequest.hpp      # HTTP request class definition
        RequestParser.hpp    # Request parsing logic
        MethodHandler.hpp    # GET/POST/DELETE handlers
        FileUpload.hpp       # Multipart form-data handling
        CGIHandler.hpp       # CGI execution logic
        ChunkedDecoder.hpp   # Chunked transfer encoding decoder

srcs/
    http/
        HTTPRequest.cpp
        RequestParser.cpp
        MethodHandler.cpp
        FileUpload.cpp
        CGIHandler.cpp
        ChunkedDecoder.cpp
```

---

## Detailed Implementation Plan

### 1. HTTPRequest Class (`HTTPRequest.hpp` / `HTTPRequest.cpp`)

**Purpose:** Store all parsed HTTP request data

**Members:**
- `std::string _method` - GET, POST, DELETE
- `std::string _uri` - Request URI (e.g., /index.html)
- `std::string _version` - HTTP/1.0 or HTTP/1.1
- `std::map<std::string, std::string> _headers` - Header key-value pairs (case-insensitive keys)
- `std::string _body` - Request body content
- `std::string _queryString` - Parsed from URI after '?'
- `bool _isComplete` - True when full request is received
- `bool _isChunked` - True if Transfer-Encoding: chunked
- `size_t _contentLength` - From Content-Length header

**Methods:**
- `std::string getMethod() const`
- `std::string getURI() const`
- `std::string getHeader(const std::string& key) const`
- `std::string getBody() const`
- `bool isComplete() const`
- `void setComplete(bool)`
- `void clear()` - Reset for reuse

---

### 2. RequestParser Class (`RequestParser.hpp` / `RequestParser.cpp`)

**Purpose:** Parse raw HTTP data into HTTPRequest object

**Key Methods:**
- `ParseState parse(const std::string& rawData, HTTPRequest& request)`
- `bool parseRequestLine(const std::string& line, HTTPRequest& request)`
- `bool parseHeaders(const std::string& headerSection, HTTPRequest& request)`
- `bool parseBody(const std::string& bodyData, HTTPRequest& request)`

**Implementation Details:**
1. **Request Line Parsing:**
   - Split by spaces: `METHOD SP URI SP VERSION CRLF`
   - Validate method is GET, POST, or DELETE
   - Extract query string from URI (split at '?')
   - Validate HTTP version

2. **Header Parsing:**
   - Read until `\r\n\r\n` (blank line)
   - Split each header at first `:` 
   - Trim whitespace from key and value
   - Store keys in lowercase for case-insensitive lookup

3. **Body Handling:**
   - Check `Transfer-Encoding: chunked` → use ChunkedDecoder
   - Check `Content-Length` → read exact bytes
   - No body indicator → request complete

4. **Incomplete Request Detection:**
   - Return NEED_MORE_DATA if request line incomplete
   - Return NEED_MORE_DATA if headers incomplete
   - Return NEED_MORE_DATA if body incomplete

**ParseState Enum:**
```cpp
enum ParseState {
    PARSE_SUCCESS,
    PARSE_INCOMPLETE,  // Need more data from socket
    PARSE_ERROR        // Malformed request
};
```

---

### 3. ChunkedDecoder Class (`ChunkedDecoder.hpp` / `ChunkedDecoder.cpp`)

**Purpose:** Decode chunked transfer encoding (RFC 7230)

**Format:**
```
<chunk-size in hex>\r\n
<chunk-data>\r\n
...
0\r\n
\r\n
```

**Methods:**
- `bool decode(const std::string& chunkedData, std::string& decodedBody)`
- `bool isComplete() const`
- `void reset()`

**Implementation:**
1. Read hex chunk size until CRLF
2. Read chunk-size bytes of data
3. Expect CRLF after chunk data
4. Repeat until chunk size is 0
5. Final CRLF marks end

---

### 4. MethodHandler Class (`MethodHandler.hpp` / `MethodHandler.cpp`)

**Purpose:** Execute HTTP methods on resources

**Methods:**
- `Response handleGET(const HTTPRequest& request, const RouteConfig& route)`
- `Response handlePOST(const HTTPRequest& request, const RouteConfig& route)`
- `Response handleDELETE(const HTTPRequest& request, const RouteConfig& route)`

**GET Implementation:**
1. Resolve file path from URI + document root
2. Check file exists and is readable
3. Read file content
4. Determine MIME type from extension
5. Return file content for response building

**POST Implementation:**
1. Check if CGI request → delegate to CGIHandler
2. Check if file upload → delegate to FileUpload
3. Otherwise process form data
4. Return appropriate response data

**DELETE Implementation:**
1. Resolve file path
2. Check file exists
3. Check delete permission (from config)
4. Remove file with `std::remove()`
5. Return success/failure status

**Method Validation:**
- Check allowed methods from route config
- Return 405 Method Not Allowed if method not permitted
- Include `Allow` header in 405 response

---

### 5. FileUpload Class (`FileUpload.hpp` / `FileUpload.cpp`)

**Purpose:** Handle multipart/form-data file uploads

**Methods:**
- `bool parse(const HTTPRequest& request, std::vector<UploadedFile>& files)`
- `bool saveFile(const UploadedFile& file, const std::string& uploadDir)`

**UploadedFile Structure:**
```cpp
struct UploadedFile {
    std::string filename;
    std::string contentType;
    std::string data;
};
```

**Implementation:**
1. **Extract Boundary:**
   - Parse from Content-Type: `multipart/form-data; boundary=----WebKitFormBoundary...`

2. **Parse Parts:**
   - Split body by `--boundary`
   - For each part, parse headers (Content-Disposition, Content-Type)
   - Extract filename from Content-Disposition
   - Extract file content after headers

3. **Save Files:**
   - Validate upload directory exists
   - Check file size against `client_max_body_size`
   - Write binary data to file
   - Handle name conflicts (optional: rename)

4. **Multiple Files:**
   - Iterate through all parts
   - Save each file individually

---

### 6. CGIHandler Class (`CGIHandler.hpp` / `CGIHandler.cpp`)

**Purpose:** Execute CGI scripts and capture output

**Methods:**
- `bool isCGIRequest(const std::string& uri, const RouteConfig& route)`
- `CGIResult execute(const HTTPRequest& request, const RouteConfig& route)`
- `char** buildEnvironment(const HTTPRequest& request)`

**CGIResult Structure:**
```cpp
struct CGIResult {
    bool success;
    int statusCode;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string errorMessage;
};
```

**Environment Variables to Set:**
- `REQUEST_METHOD` - GET, POST, etc.
- `QUERY_STRING` - From URI after '?'
- `CONTENT_TYPE` - From request header
- `CONTENT_LENGTH` - Body size
- `PATH_INFO` - Extra path after script
- `SCRIPT_FILENAME` - Full path to CGI script
- `SCRIPT_NAME` - URI path to script
- `SERVER_NAME` - From Host header
- `SERVER_PORT` - Listening port
- `SERVER_PROTOCOL` - HTTP/1.1
- `GATEWAY_INTERFACE` - CGI/1.1
- `HTTP_*` - All HTTP headers prefixed

**Implementation Steps:**
1. **Setup Pipes:**
   ```cpp
   int stdin_pipe[2];   // Parent writes request body
   int stdout_pipe[2];  // Parent reads CGI output
   pipe(stdin_pipe);
   pipe(stdout_pipe);
   ```

2. **Fork Process:**
   ```cpp
   pid_t pid = fork();
   ```

3. **Child Process:**
   ```cpp
   // Redirect stdin to pipe
   dup2(stdin_pipe[0], STDIN_FILENO);
   close(stdin_pipe[1]);
   
   // Redirect stdout to pipe
   dup2(stdout_pipe[1], STDOUT_FILENO);
   close(stdout_pipe[0]);
   
   // Execute CGI
   execve(cgi_path, argv, envp);
   ```

4. **Parent Process:**
   ```cpp
   // Close unused ends
   close(stdin_pipe[0]);
   close(stdout_pipe[1]);
   
   // Write request body to CGI
   write(stdin_pipe[1], body.c_str(), body.size());
   close(stdin_pipe[1]);
   
   // Read CGI output
   read(stdout_pipe[0], buffer, size);
   close(stdout_pipe[0]);
   
   // Wait for child
   waitpid(pid, &status, 0);
   ```

5. **Timeout Handling:**
   - Use `alarm()` or non-blocking I/O with select/poll
   - Kill child process if timeout exceeded
   - Return 504 Gateway Timeout

6. **Parse CGI Output:**
   - Headers end at first blank line
   - Parse `Status:` header for status code
   - Rest is response body

---

## Integration Points

### With Person 1 (Server Core):
- Person 1 calls `RequestParser::parse()` when data arrives on socket
- Return PARSE_INCOMPLETE tells Person 1 to wait for more data
- Return PARSE_SUCCESS provides complete HTTPRequest object
- Person 1 manages the socket, I provide parsing logic

### With Person 3 (Config & Response):
- Receive `RouteConfig` from Person 3's routing system
- Use `client_max_body_size` from config for validation
- Use `allowed_methods` from config for 405 checks
- Use `cgi_pass` from config for CGI binary path
- Use `upload_store` from config for upload directory
- Pass response data to Person 3's Response builder

---

## Testing Strategy

### Unit Tests:
1. Parse valid GET request
2. Parse valid POST with body
3. Parse chunked encoding
4. Parse multipart form-data
5. Handle malformed requests
6. Handle incomplete requests

### Integration Tests:
1. Test with curl commands
2. Test with browser uploads
3. Test CGI with PHP/Python scripts
4. Stress test with large files
5. Compare output with nginx

### Test Commands:
```bash
# Simple GET
curl -v http://localhost:8080/index.html

# POST with body
curl -v -X POST -d "data=test" http://localhost:8080/submit

# File upload
curl -v -F "file=@test.txt" http://localhost:8080/upload

# DELETE
curl -v -X DELETE http://localhost:8080/file.txt

# Chunked encoding
curl -v -H "Transfer-Encoding: chunked" -d @largefile.txt http://localhost:8080/upload
```

---

## Development Order

1. **Week 1:** HTTPRequest + RequestParser (basic GET parsing)
2. **Week 2:** POST/DELETE methods + ChunkedDecoder
3. **Week 3:** FileUpload + CGIHandler
4. **Week 4:** Integration, testing, bug fixes

---

## C++98 Reminders

- No `auto` keyword
- No range-based for loops
- No `nullptr` (use `NULL`)
- No `std::to_string()` (use `std::stringstream`)
- No `std::stoi()` (use `std::atoi()` or `std::strtol()`)
- No initializer lists for containers
- Use `iterator` explicitly in loops

---

## Notes

- **CRITICAL:** All socket I/O must go through poll() - Person 1's responsibility
- Never block on read/write - return incomplete status instead
- Keep parsing stateless where possible for easier debugging
- Document all assumptions about config structure with Person 3

---

## WHAT YOU DEAL WITH - DETAILED EXPLANATION

### Your Role in Simple Terms
You are the "HTTP Request Expert". Your job is to take raw bytes that arrive 
on sockets and convert them into structured data that the rest of the server 
understands.

---

### Your Data Pipeline

**INPUT:** Raw bytes from socket (from Person 1's poll())
```
GET /index.html HTTP/1.1\r\n
Host: localhost\r\n
Content-Type: text/html\r\n
\r\n
```

**PROCESSING:** Parse, validate, decode, extract metadata
- `"GET /index.html HTTP/1.1"` → method=GET, uri=/index.html, version=HTTP/1.1
- Headers → case-insensitive map
- Body → handle Content-Length or chunked encoding

**OUTPUT:** Structured HTTPRequest object ready for routing
+ Handler results (file content for GET, upload status for POST, etc.)
→ Person 3's Response Builder takes this to construct HTTP response

---

### The 6 Components Explained

#### 1. HTTPRequest CLASS
- **What it is:** A data container (just holds stuff)
- **What it holds:** method, URI, version, headers (map), body, query string
- **Who uses it:** Everyone - it's the central object passed around
- **Your job:** Provide getter methods and validation checks

#### 2. RequestParser CLASS
- **What it is:** The main parser engine
- **What it does:** Takes raw HTTP data and fills HTTPRequest objects
- **Key challenge:** Detect INCOMPLETE requests (Person 1 sends partial data via poll)
- **Your job:** Return PARSE_INCOMPLETE if you need more bytes from socket
- **Your job:** Return PARSE_SUCCESS when full request arrives

#### 3. ChunkedDecoder CLASS
- **What it is:** A specialized decoder for one specific thing
- **What it handles:** "Transfer-Encoding: chunked" format (RFC 7230)
- **Format explanation:**
  - Client sends body in chunks instead of one blob
  - Each chunk has a hex size: `"a\r\n"` (10 bytes coming)
  - Then the 10 bytes of data
  - Then `"\r\n"`
  - Repeat until `"0\r\n\r\n"`
- **Your job:** Un-chunk it → give normal body to CGI/form handlers

#### 4. MethodHandler CLASS
- **What it is:** Handler for GET, POST, DELETE operations
- **What it does:** Takes HTTPRequest + routing config, returns operation result
- **GET:** Read file from disk → return file content for response
- **POST:** If file upload → call FileUpload, if CGI → call CGIHandler
- **DELETE:** Remove file from disk → return success/failure
- **Your job:** Work with file system, enforce config rules (allowed_methods, etc.)

#### 5. FileUpload CLASS
- **What it is:** Multipart/form-data decoder
- **What it handles:** Browser file uploads
- **Input format:** Ugly boundary-separated chunks with metadata
- **Your job:**
  - Find boundary string from Content-Type header
  - Split body by boundary
  - For each part: extract filename and binary file data
  - Save each file to upload directory
  - Validate file size < client_max_body_size
  - Return list of saved files

#### 6. CGIHandler CLASS
- **What it is:** External program executor
- **What it does:** Run PHP/Python/shell scripts and capture their output
- **Your job:**
  - Detect if request is for CGI (file extension = .php, .py, etc.)
  - Set up environment variables (REQUEST_METHOD, QUERY_STRING, etc.)
  - Fork a child process
  - Pipe request body to child's stdin
  - Read child's stdout (headers + body)
  - Parse CGI response headers
  - Handle timeout (child takes too long)
  - Wait for child process to finish
  - Return CGI output for response building

---

### Data Flow Examples

#### EXAMPLE 1: Simple GET
1. Browser sends: `GET /index.html HTTP/1.1`
2. Person 1 receives bytes via poll() → calls `RequestParser::parse()`
3. RequestParser extracts: method=GET, uri=/index.html
4. `MethodHandler::handleGET()` reads file from disk
5. Returns file content to Response builder
6. Response sends 200 OK with file

#### EXAMPLE 2: File Upload
1. Browser sends: POST /upload with multipart/form-data + photo.jpg
2. Person 1 receives bytes → RequestParser parses headers
3. Detects `Content-Type: multipart/form-data`
4. `MethodHandler::handlePOST()` calls `FileUpload::parse()`
5. FileUpload extracts boundary, finds photo.jpg data
6. Saves photo.jpg to upload directory
7. Returns success to Response builder
8. Response sends 200 OK "File uploaded"

#### EXAMPLE 3: CGI Script
1. Browser sends: `GET /test.php?name=john HTTP/1.1`
2. Person 1 receives bytes → RequestParser parses
3. MethodHandler detects .php extension → calls CGIHandler
4. CGIHandler forks, sets env: REQUEST_METHOD=GET, QUERY_STRING=name=john
5. Child process executes `/usr/bin/php /path/to/test.php`
6. CGI script outputs: `"Content-Type: text/html\r\n\r\n<html>..."`
7. CGIHandler parses headers and body separately
8. Returns parsed output to Response builder
9. Response sends back the HTML

#### EXAMPLE 4: Chunked Body (POST with chunked upload)
1. Browser sends: POST /upload with `Transfer-Encoding: chunked`
2. Body arrives as: `"a\r\nHelloWorld\r\n5\r\nFinal\r\n0\r\n\r\n"`
3. RequestParser detects chunked encoding
4. Calls `ChunkedDecoder::decode()`
5. ChunkedDecoder returns: `"HelloWorldFinal"` (unchunked)
6. Rest of handlers work with normal body

---

### Key Responsibilities

1. **NEVER BLOCK** waiting for socket data
   - Return PARSE_INCOMPLETE if you need more bytes
   - Person 1 will call you again when more data arrives

2. **HANDLE INCOMPLETE REQUESTS** correctly
   - Request line might arrive in multiple packets
   - Headers might arrive split across packets
   - Body might arrive in chunks (especially POST data)

3. **CASE-INSENSITIVE HEADERS**
   - "Content-Type" = "content-type" = "CONTENT-TYPE"
   - Store all header keys in lowercase for consistency

4. **VALIDATE EVERYTHING**
   - Check method is GET, POST, or DELETE (reject others with 501)
   - Check HTTP version is HTTP/1.0 or HTTP/1.1
   - Check Content-Length or Transfer-Encoding is present for body
   - Check file size < client_max_body_size

5. **WORK WITH PERSON 3's CONFIG**
   - Get allowed_methods from route config (enforce 405 errors)
   - Get client_max_body_size limit
   - Get cgi_pass (CGI executable path)
   - Get upload_store directory path

6. **HANDLE ERRORS GRACEFULLY**
   - Malformed request → 400 Bad Request
   - Method not allowed → 405 Method Not Allowed
   - File too large → 413 Payload Too Large
   - CGI timeout → 504 Gateway Timeout
   - CGI execution error → 502 Bad Gateway

---

### Who Calls You and What Do You Return

**Person 1 (Server Core):**
- Calls: `RequestParser::parse(raw_bytes, request_object)`
- Returns: `PARSE_SUCCESS` / `PARSE_INCOMPLETE` / `PARSE_ERROR`

**Person 3 (Router/Config):**
- Calls: `MethodHandler::handleGET/POST/DELETE(request, route_config)`
- Returns: Handler result (file content, upload list, delete status)

- Calls: `FileUpload::parse(request, files_vector)`
- Returns: success/failure + list of uploaded files

- Calls: `CGIHandler::execute(request, route_config)`
- Returns: CGI output (headers + body as strings)
