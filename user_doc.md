*This project has been created as part of the 42 curriculum by [login1], [login2], [login3].*

# Webserv - HTTP Server in C++98

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-98-blue" />
  <img src="https://img.shields.io/badge/HTTP-1.1-green" />
  <img src="https://img.shields.io/badge/Status-In%20Development-yellow" />
</p>

## Description

**Webserv** is a custom HTTP server implementation written in C++98, developed as part of the 42 Network curriculum. This project provides a deep understanding of the HTTP protocol, network programming, and non-blocking I/O operations.

### Project Goals

- Implement a fully functional HTTP/1.1 server from scratch
- Handle multiple simultaneous client connections using I/O multiplexing
- Serve static websites and dynamic content through CGI
- Parse and respond to HTTP requests according to RFC specifications
- Manage file uploads, downloads, and deletions
- Process configuration files similar to NGINX

### Key Features

- ✅ **HTTP Methods**: GET, POST, DELETE
- ✅ **Non-blocking I/O**: Single `poll()`/`select()`/`kqueue()`/`epoll()` for all connections
- ✅ **Static File Serving**: Deliver HTML, CSS, JavaScript, images, and more
- ✅ **CGI Support**: Execute Python, PHP, and other CGI scripts
- ✅ **File Upload/Download**: Handle multipart form data
- ✅ **Multiple Ports**: Listen on multiple ports simultaneously
- ✅ **Directory Listing**: Automatic index generation (autoindex)
- ✅ **Error Pages**: Custom and default error page handling
- ✅ **Configuration Files**: NGINX-style configuration parsing
- ✅ **Request Body Size Limits**: Configurable maximum body size
- ✅ **HTTP Redirections**: 301/302 redirects
- 🎁 **Bonus**: Cookie/session management, multiple CGI types

---

## Instructions

### Prerequisites

- C++ compiler with C++98 support (`g++`, `clang++`)
- UNIX-based operating system (Linux, macOS)
- Python 3 or PHP-CGI (for CGI testing)
- `make` utility

### Compilation

```bash
# Clone the repository
git clone <repository-url>
cd webserv

# Compile the project
make

# The executable 'webserv' will be created
```

### Makefile Rules

- `make` or `make all` - Compile the project
- `make clean` - Remove object files
- `make fclean` - Remove object files and executable
- `make re` - Recompile the entire project

### Running the Server

```bash
# Run with default configuration
./webserv

# Run with custom configuration file
./webserv config/custom.conf
```

The server will start and listen on the ports specified in the configuration file (default: 8080).

### Testing the Server

#### Browser Testing
1. Start the server: `./webserv`
2. Open your browser and navigate to:
   - `http://localhost:8080` - Main page
   - `http://localhost:8080/uploads` - File upload interface
   - `http://localhost:8080/cgi-bin/test.py` - CGI test

#### Command Line Testing

```bash
# Test with curl
curl http://localhost:8080
curl -X POST -F "file=@test.txt" http://localhost:8080/upload
curl -X DELETE http://localhost:8080/file.txt

# Test with telnet (raw HTTP)
telnet localhost 8080
GET / HTTP/1.1
Host: localhost
<press Enter twice>

# Stress testing
siege -c 100 -r 10 http://localhost:8080
ab -n 1000 -c 50 http://localhost:8080/
```

---

## Configuration File

The server uses an NGINX-inspired configuration format. Example:

```nginx
server {
    listen 8080;
    server_name localhost;
    client_max_body_size 10M;
    
    error_page 404 /error/404.html;
    error_page 500 /error/500.html;
    
    location / {
        root www/html;
        index index.html index.htm;
        allowed_methods GET POST;
        autoindex on;
    }
    
    location /uploads {
        root www/uploads;
        allowed_methods GET POST DELETE;
        upload_pass www/uploads;
    }
    
    location /redirect {
        return 301 /;
    }
    
    location /cgi-bin {
        root www/cgi-bin;
        allowed_methods GET POST;
        cgi_pass .py /usr/bin/python3;
        cgi_pass .php /usr/bin/php-cgi;
    }
}

server {
    listen 8081;
    server_name example.com;
    
    location / {
        root www/site2;
        index index.html;
    }
}
```

### Configuration Directives

- **listen**: Port to listen on (e.g., `8080`, `0.0.0.0:8080`)
- **server_name**: Server name for virtual host matching
- **client_max_body_size**: Maximum allowed request body size
- **error_page**: Custom error pages (e.g., `error_page 404 /404.html`)
- **root**: Root directory for serving files
- **index**: Default files to serve for directories
- **allowed_methods**: HTTP methods allowed for this location
- **autoindex**: Enable/disable directory listing (on/off)
- **upload_pass**: Directory where uploaded files are stored
- **return**: HTTP redirection (e.g., `return 301 /new-path`)
- **cgi_pass**: CGI interpreter mapping (e.g., `.php /usr/bin/php-cgi`)

---

## Project Structure

```
webserv/
├── Makefile
├── README.md
├── config/
│   ├── default.conf          # Default server configuration
│   └── test.conf             # Testing configuration
├── include/
│   ├── Server.hpp            # Main server class
│   ├── Client.hpp            # Client connection handler
│   ├── HTTPRequest.hpp       # HTTP request representation
│   ├── HTTPParser.hpp        # HTTP request parser
│   ├── HTTPResponse.hpp      # HTTP response builder
│   ├── Config.hpp            # Configuration file parser
│   ├── FileServer.hpp        # Static file handler
│   ├── CGIHandler.hpp        # CGI execution handler
│   └── Utils.hpp             # Utility functions
├── src/
│   ├── main.cpp              # Entry point
│   ├── server/
│   │   ├── Server.cpp        # Server implementation
│   │   └── Client.cpp        # Client implementation
│   ├── http/
│   │   ├── HTTPRequest.cpp
│   │   ├── HTTPParser.cpp
│   │   ├── HTTPResponse.cpp
│   │   ├── CGIHandler.cpp
│   │   └── FileServer.cpp
│   ├── config/
│   │   └── Config.cpp        # Configuration parsing
│   └── utils/
│       └── Utils.cpp
├── www/
│   ├── html/                 # Static website files
│   │   ├── index.html
│   │   └── error/
│   │       ├── 404.html
│   │       └── 500.html
│   ├── uploads/              # Upload directory
│   └── cgi-bin/              # CGI scripts
│       ├── test.py
│       └── test.php
└── tests/
    └── test_requests.py      # Automated testing scripts
```

---

## Technical Implementation

### I/O Multiplexing

The server uses a single `poll()` (or equivalent) call to monitor all file descriptors:
- Listening sockets (accept new connections)
- Client sockets (read requests, write responses)
- No blocking operations on sockets (all set to `O_NONBLOCK`)

**Critical Requirement**: All socket I/O operations MUST go through `poll()` readiness checks. Direct `read()`/`write()` without checking readiness results in automatic failure.

### HTTP Request Processing Flow

```
Client Connect → poll() signals → accept() → Create Client object
                                                      ↓
Client sends data → poll() POLLIN → recv() → Append to buffer
                                                      ↓
                                            Parse HTTP request
                                                      ↓
                              ┌──────────────────────┼──────────────────────┐
                              ↓                      ↓                      ↓
                         Static File              CGI Script           File Upload
                              ↓                      ↓                      ↓
                       Read from disk      fork() + execve()      Parse multipart
                              ↓                      ↓                      ↓
                         Build Response        Parse CGI output      Save to disk
                              └──────────────────────┼──────────────────────┘
                                                     ↓
                                         Queue response in buffer
                                                     ↓
                            poll() POLLOUT → send() → Close or keep-alive
```

### HTTP Methods

- **GET**: Retrieve resources (files, directory listings, CGI output)
- **POST**: Upload files, submit forms, send data to CGI
- **DELETE**: Remove files from the server

### CGI (Common Gateway Interface)

CGI scripts are executed in child processes with proper environment variables:
- `REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_TYPE`, `CONTENT_LENGTH`
- `PATH_INFO`, `SCRIPT_FILENAME`, `SERVER_PROTOCOL`, `HTTP_*` headers

The server handles:
- Forking child processes
- Piping request body to CGI stdin
- Reading CGI stdout for response
- Parsing CGI headers and body
- Timeout handling for misbehaving scripts

---

## Resources

### HTTP Protocol
- [RFC 2616 - HTTP/1.1](https://www.rfc-editor.org/rfc/rfc2616) - Official HTTP/1.1 specification
- [RFC 7230-7235](https://tools.ietf.org/html/rfc7230) - Updated HTTP/1.1 specifications
- [Mozilla HTTP Documentation](https://developer.mozilla.org/en-US/docs/Web/HTTP) - Comprehensive HTTP guide
- [HTTP Made Really Easy](https://www.jmarshall.com/easy/http/) - Beginner-friendly HTTP tutorial

### Network Programming
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) - Socket programming in C
- [poll() Man Page](https://man7.org/linux/man-pages/man2/poll.2.html) - I/O multiplexing documentation
- [select() Tutorial](https://www.gnu.org/software/libc/manual/html_node/Waiting-for-I_002fO.html) - Alternative to poll()
- [The C10K Problem](http://www.kegel.com/c10k.html) - Handling many concurrent connections

### Configuration
- [NGINX Configuration](https://nginx.org/en/docs/beginners_guide.html) - Configuration file reference
- [NGINX Server Blocks](https://nginx.org/en/docs/http/ngx_http_core_module.html) - Understanding server contexts

### CGI
- [CGI Specification](https://www.rfc-editor.org/rfc/rfc3366) - Common Gateway Interface standard
- [CGI Environment Variables](https://www.w3.org/CGI/) - W3C CGI documentation
- [Writing CGI Scripts](https://httpd.apache.org/docs/2.4/howto/cgi.html) - Apache CGI tutorial

### Testing Tools
- [curl Manual](https://curl.se/docs/manual.html) - HTTP client testing
- [Postman](https://www.postman.com/) - API testing tool
- [Apache Bench (ab)](https://httpd.apache.org/docs/2.4/programs/ab.html) - HTTP server benchmarking
- [Siege](https://www.joedog.org/siege-home/) - Stress testing tool

### C++98 Reference
- [C++ Reference](https://en.cppreference.com/w/cpp/98) - C++98 standard library
- [Socket Programming in C++](https://www.geeksforgeeks.org/socket-programming-cc/) - Tutorial

---

## AI Usage Disclosure

AI tools (ChatGPT, Claude, GitHub Copilot) were used in the following capacities during this project:

### Tasks Where AI Was Used

1. **Code Structure Planning**
   - Generated initial class hierarchies and module organization
   - Suggested design patterns for request/response handling
   - Reviewed and discussed with team before implementation

2. **Configuration Parser Design**
   - Assisted with tokenization logic for NGINX-style config files
   - Provided examples of state machine approaches
   - All code was rewritten and fully understood by team

3. **HTTP RFC Clarification**
   - Explained specific HTTP header behaviors
   - Clarified chunked transfer encoding implementation
   - Helped understand Content-Length vs Transfer-Encoding

4. **Error Handling Patterns**
   - Suggested C++98-compliant exception handling approaches
   - Provided examples of errno handling (though avoided per requirements)
   - Discussed graceful degradation strategies

5. **Testing Script Generation**
   - Generated initial Python test scripts for HTTP requests
   - Created curl command examples for manual testing
   - Scripts were modified based on our specific implementation

### What We Implemented Ourselves

- **All core logic**: Server loop, poll() implementation, request parsing
- **Protocol implementation**: HTTP request/response handling from scratch
- **CGI execution**: Fork/exec logic, pipe handling, environment variables
- **Configuration parsing**: Custom parser written and debugged by team
- **Integration**: All components integrated and tested manually

### Verification Process

Every piece of AI-generated code or suggestion was:
1. Reviewed by all team members in group sessions
2. Tested individually before integration
3. Rewritten when not fully understood
4. Debugged and modified to fit our architecture
5. Explained during peer code reviews

---

## Team & Contributions

### Division of Labor

**Person 1 - Core Server & I/O Multiplexing**
- Socket setup and management
- poll()/select() event loop implementation
- Connection acceptance and client lifecycle
- Non-blocking I/O handling
- Timeout management

**Person 2 - HTTP Parser & Request Processing**
- HTTP request parsing (method, headers, body)
- GET, POST, DELETE method handlers
- File upload parsing (multipart/form-data)
- CGI integration and execution
- Chunked transfer encoding

**Person 3 - Configuration & Response Building**
- Configuration file parser (NGINX-style)
- Route resolution and matching
- HTTP response builder
- Static file serving
- Error page generation
- Directory listing (autoindex)

### Shared Responsibilities
- Integration testing
- Documentation
- Makefile and build system
- Debugging and optimization
- Cross-browser compatibility testing
- Stress testing and performance tuning

---

## Testing & Validation

### Manual Testing Checklist

- [ ] Server starts without crashes
- [ ] Accepts connections on configured ports
- [ ] Serves static HTML files correctly
- [ ] Handles CSS, JavaScript, images with proper MIME types
- [ ] GET requests return 200 for existing files
- [ ] GET requests return 404 for missing files
- [ ] POST file uploads work and save to correct directory
- [ ] DELETE removes files with proper permissions
- [ ] Directory listing (autoindex) generates valid HTML
- [ ] CGI scripts execute and return correct output
- [ ] Error pages display for 4xx and 5xx errors
- [ ] Redirects (301/302) work as configured
- [ ] Request body size limit enforced
- [ ] Multiple simultaneous connections handled
- [ ] Keep-alive connections work
- [ ] Server doesn't crash with malformed requests
- [ ] Timeout handling prevents hung connections

### Stress Testing

```bash
# Test with 100 concurrent connections
siege -c 100 -r 10 http://localhost:8080

# Apache Bench test
ab -n 10000 -c 100 http://localhost:8080/

# Memory leak check
valgrind --leak-check=full ./webserv config/default.conf

# Test with large file uploads
dd if=/dev/zero of=test10mb.bin bs=1M count=10
curl -X POST -F "file=@test10mb.bin" http://localhost:8080/upload
```

---

## Known Limitations & Future Improvements

### Current Limitations
- HTTP/1.1 only (no HTTP/2 support)
- No TLS/SSL support (no HTTPS)
- Limited virtual host support
- Single-threaded (though handles multiple connections)
- No request pipelining

### Potential Improvements
- Add HTTPS support with OpenSSL
- Implement HTTP/2 protocol
- Add caching mechanisms (ETag, Last-Modified)
- Implement request pipelining
- Add WebSocket support
- Improve performance with thread pool
- Add logging to files with rotation
- Implement rate limiting
- Add gzip compression support

---

## License

This project is created for educational purposes as part of the 42 Network curriculum.

---

## Acknowledgments

- 42 Network for the project subject
- NGINX team for configuration file inspiration
- The creators of RFC 2616 and HTTP specifications
- Our peers for code reviews and testing assistance
- Open source community for documentation and tools

---

**Note**: This server is built for educational purposes and should not be used in production environments without significant security hardening and additional features.