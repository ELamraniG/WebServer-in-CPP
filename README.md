*This project has been created as part of the 42 curriculum by mboutahi, moel-amr, mdaghouj

## Description

**webserv** is a fully-featured HTTP/1.0 web server implemented in C++98, created from scratch without relying on external frameworks or libraries. This project demonstrates a deep understanding of network programming, HTTP protocol mechanics, and system-level programming concepts.

The server is built to handle multiple simultaneous client connections using an event-driven architecture with `poll()`. It supports:
- **Static file serving** with directory indexing (optional)
- **Dynamic content generation** through CGI (Common Gateway Interface) script execution
- **File uploads and downloads** with configurable size limits
- **Custom error pages** for various HTTP status codes
- **Configuration file parsing** for flexible server setup
- **Multiple server blocks** on different ports and hosts
- **Chunked transfer encoding** support
- **Request timeouts** and connection management
- **Comprehensive logging** with color-coded status indicators

---

## Features

### Core Server Capabilities
- **Multi-client support** using `poll()` for event-driven I/O
- **HTTP/1.0** protocol support
- **Non-Persistent Connections** "One request per connection" model
- **Chunked transfer encoding** for request bodies
- **Large file uploads** with configurable size limits
- **Directory autoindexing** with customizable HTML templates

### HTTP Methods
- **GET** - Retrieve static files or execute CGI scripts
- **POST** - Upload files and submit form data
- **DELETE** - Remove files from designated upload directories

### CGI Integration
- **Python** script execution
- **PHP** script execution
- **Non-blocking pipe management** for concurrent CGI operations
- **CGI timeout handling** with configurable limits
- **Environment variable passing** (REQUEST_METHOD, QUERY_STRING, CONTENT_LENGTH, HTTP_* headers, etc.)

### Configuration
- **Nginx-like configuration syntax** for server blocks and location directives
- **Per-location settings** (allowed methods, file root, index files, redirects, CGI handlers)
- **Custom error pages** mapped to HTTP status codes
- **Client max body size** enforcement
- **Host-based virtual hosting** support

---

## Instructions

### Prerequisites
- **C++98 compatible compiler** (c++, clang++)
- **Unix-like operating system** (Linux, macOS)
- **Make** build tool
- **Python3** (optional, for Python CGI scripts)
- **PHP-CGI** (optional, for PHP script support)

### Compilation

```bash
cd /path/to/webserv
make
```

This will generate the executable `webserv` in the project root.

**Clean build:**
```bash
make clean      # Remove object files
make fclean     # Remove all generated files
make re         # Clean and rebuild
```

### Execution

```bash
./webserv <config_file>
```

**Example:**
```bash
./webserv config/webserv.conf
```

The server will start listening on the addresses specified in the configuration file. By default, it listens on:
- `127.0.0.1:8080` (primary server with full features)
- `127.0.0.1:8081` (secondary server example)

### Configuration File Syntax

The configuration file uses an Nginx-like syntax:

```conf
server {
    listen 127.0.0.1:8080;          # Host and port to listen on
    root www;                        # Root directory for serving files
    index pages/index.html;          # Default index file
    client_max_body_size 10M;        # Maximum upload size

    # Error pages mapping
    error_page 404 errors/404.html;
    error_page 500 errors/500.html;

    # Route configuration
    location / {
        allowed_methods GET;         # Comma-separated: GET, POST, DELETE
        autoindex off;               # Enable/disable directory listing
        index pages/index.html;      # Location-specific index
    }

    # CGI configuration
    location /cgi-bin {
        allowed_methods GET POST;
        cgi_pass .py /usr/bin/python3;  # Map file extensions to interpreters
        cgi_pass .php /usr/bin/php-cgi;
    }

    # File uploads
    location /uploads {
        allowed_methods GET POST DELETE;
        upload_pass www/uploads;     # Directory for uploaded files
        autoindex on;
    }

    # Redirects
    location /old {
        return 301 /new;             # 301 Moved Permanently
        # Also supports 302, 307, 308
    }
}
```

### Usage Examples

#### Serving Static Files
```bash
# Simple GET request
curl http://127.0.0.1:8080/pages/index.html

# Directory listing (if autoindex is on)
curl http://127.0.0.1:8080/pages/
```

#### Uploading Files
```bash
# Using the upload form
curl -F "file=@/path/to/file.txt" http://127.0.0.1:8080/uploads

# Or directly via POST
curl --upload-file /path/to/file.txt http://127.0.0.1:8080/uploads/
```

#### Executing CGI Scripts
```bash
# Python script with query string
curl "http://127.0.0.1:8080/cgi-bin/script.py?name=value&other=data"

# POST to CGI script
curl -X POST -d "key1=value1&key2=value2" http://127.0.0.1:8080/cgi-bin/script.py
```

#### Testing Redirects
```bash
curl -L http://127.0.0.1:8080/old
```

#### Testing Error Handling
```bash
# 404 Not Found
curl http://127.0.0.1:8080/nonexistent

# 405 Method Not Allowed
curl -X DELETE http://127.0.0.1:8080/

# 413 Payload Too Large (if upload exceeds limit)
dd if=/dev/zero bs=1M count=20 | curl -X POST --data-binary @- http://127.0.0.1:8080/uploads
```

### Stopping the Server
Press `Ctrl+C` to gracefully shutdown the server. The server handles `SIGINT` and `SIGTERM` signals for clean termination.

---

## Project Structure

```
webserv/
├── main.cpp                 # Server entry point
├── Makefile                 # Build configuration
├── config/
│   └── webserv.conf        # Server configuration file
├── include/                # Header files (.hpp)
│   ├── cgi/                # CGI handler interface
│   ├── config/             # Configuration parsing
│   ├── core/               # Core server components
│   ├── http/               # HTTP protocol handling
│   └── logger/             # Logging utilities
├── src/                    # Implementation files (.cpp)
│   ├── cgi/
│   ├── config/
│   ├── core/
│   ├── http/
│   └── logger/
└── www/                    # Web root directory
    ├── pages/              # Static HTML pages
    ├── cgi-bin/            # CGI scripts
    ├── uploads/            # Upload destination
    └── errors/             # Error page templates
```

---

## Resources

### HTTP Protocol References
- [RFC 7230 - HTTP/1.0 Message Syntax and Routing](https://tools.ietf.org/html/rfc7230)
- [RFC 7231 - HTTP/1.0 Semantics and Content](https://tools.ietf.org/html/rfc7231)
- [MDN HTTP Documentation](https://developer.mozilla.org/en-US/docs/Web/HTTP)

### Socket Programming
- [POSIX.1-2017 Sockets API](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_socket.h.html)
- [man 2 poll](https://man7.org/linux/man-pages/man2/poll.2.html)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

### CGI Specification
- [RFC 3875 - The Common Gateway Interface (CGI)](https://tools.ietf.org/html/rfc3875)
- [CGI Tutorial](https://www.tutorialspoint.com/python_cgi/python_cgi_introduction.htm)

### C++ and System Programming
- [C++98 Standard Reference](https://en.cppreference.com/w/cpp/compiler_support/98)
- [Linux man pages online](https://man7.org/linux/man-pages/)
- [The Linux Programming Interface](https://man7.org/tlpi/) - Comprehensive reference

### Nginx Configuration Reference
- [Nginx Configuration Syntax](https://nginx.org/en/docs/syntax.html)
- Used as inspiration for the server block and location configuration syntax

---

## AI Usage & Assistance

AI was utilized strategically for educational purposes:

---

### Tasks Where AI Was Used:
AI was used for:
- searching resources about HTTP/1.0
- correct grammar for README.md.
- searching core concepts.
- generating tests and validations.

---
