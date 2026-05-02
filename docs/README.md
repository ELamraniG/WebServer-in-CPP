# WebServer Documentation Index

This directory contains one Markdown file per class used in the project.  
Each file explains the class's **purpose**, **problem it solves**, and a detailed breakdown of **every major function**.

---

## Architecture Overview

```
Config Pipeline:        Tokenizer → Parser → Server_block / location_block
                                                    ↓
Server Startup:                               Server (TCP socket setup)
                                                    ↓
Runtime:    EventLoop (poll loop)
                ├── Client         (per-connection socket + buffers)
                ├── RequestParser  (raw bytes → HTTPRequest)
                │       └── ChunksDecoding  (chunked body decoding)
                ├── Router         (match server block + location block)
                ├── RouteConfig    (unified config view with fallback)
                ├── MethodHandler  (GET / POST / DELETE logic)
                │       ├── FileUpload  (multipart file upload)
                │       └── Response   (typed response container)
                ├── ResponseBuilder (Response → raw HTTP string)
                ├── CGIHandler     (fork/exec CGI scripts via pipes)
                └── Logger         (colorized terminal output)
```

---

## Class Files

| File | Class / Struct | Layer | Purpose |
|---|---|---|---|
| [Tokenizer.md](Tokenizer.md) | `Tokenizer` | Config | Reads `.conf` file, strips comments, produces token list |
| [Parser.md](Parser.md) | `Parser` | Config | Builds structured `Server_block` objects from tokens |
| [Server_block_and_location_block.md](Server_block_and_location_block.md) | `Server_block`, `location_block` | Config | Plain data holders for configuration values |
| [Server.md](Server.md) | `Server` | Core | TCP socket: create, bind, listen, accept |
| [Client.md](Client.md) | `Client` | Core | Per-connection: read buffer, write buffer, timeout |
| [EventLoop.md](EventLoop.md) | `EventLoop` | Core | `poll()` event loop: dispatch reads, writes, CGI, timeouts |
| [HTTPRequest.md](HTTPRequest.md) | `HTTPRequest` | HTTP | Typed container for a parsed HTTP request |
| [RequestParser.md](RequestParser.md) | `RequestParser` | HTTP | Converts raw bytes → `HTTPRequest`, handles chunked bodies |
| [ChunksDecoding.md](ChunksDecoding.md) | `ChunksDecoding` | HTTP | Decodes `Transfer-Encoding: chunked` request bodies |
| [Router.md](Router.md) | `Router` | HTTP | Matches request to `Server_block` and `location_block` |
| [RouteConfig.md](RouteConfig.md) | `RouteConfig` | HTTP | Merges server + location config with inheritance fallback |
| [MethodHandler.md](MethodHandler.md) | `MethodHandler` | HTTP | GET/POST/DELETE logic: files, uploads, autoindex, sessions |
| [FileUpload.md](FileUpload.md) | `FileUpload` | HTTP | Parses multipart form data and saves files to disk |
| [Response.md](Response.md) | `Response` | HTTP | Typed intermediate response (status, type, body, headers) |
| [ResponseBuilder.md](ResponseBuilder.md) | `ResponseBuilder` | HTTP | Serializes `Response` → raw HTTP/1.0 string |
| [CGIHandler.md](CGIHandler.md) | `CGIHandler` | CGI | fork/exec CGI scripts, non-blocking pipe I/O |
| [Logger.md](Logger.md) | `Logger` | Util | Colorized ANSI terminal logging for all server events |
