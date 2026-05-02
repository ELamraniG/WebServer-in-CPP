# Server_block and location_block

## Overview

`Server_block` and `location_block` are simple **data-holder structs** (Plain Old Data). They have no logic — they exist purely to store the parsed configuration values after `Parser` has processed the config file. Every other class (`Server`, `EventLoop`, `RouteConfig`, `Router`) reads from these structs to make decisions.

**Problem they solve:** Configuration data needs to flow from the parser (which runs once at startup) to every part of the server runtime (event loop, request handling, CGI, etc.). Using plain structs keeps the data model simple and decoupled from any single component.

---

## `Server_block`

Represents one `server { }` block from the config file.

### Fields

| Field | Type | Default | Purpose |
|---|---|---|---|
| `port` | `string` | `""` | The TCP port to listen on (e.g., `"8080"`) |
| `host` | `string` | `"0.0.0.0"` | The IP address to bind to; `"0.0.0.0"` means all interfaces |
| `root` | `string` | `""` | The filesystem root directory for serving files |
| `index` | `string` | `""` | Default file to serve when a directory is requested (e.g., `"index.html"`) |
| `error_pages` | `map<int, string>` | empty | Maps HTTP error codes to custom HTML file paths (e.g., `{404 → "/errors/404.html"}`) |
| `client_max_body_size` | `unsigned long` | `1048576` (1 MB) | Maximum allowed request body size in bytes |
| `locations` | `vector<location_block>` | empty | All `location { }` blocks nested inside this server block |

### Constructor
The default constructor sets safe defaults:
- `host = "0.0.0.0"` — listen on all interfaces.
- `client_max_body_size = 1048576` — 1 MB default limit.
- All strings set to empty.

---

## `location_block`

Represents one `location /path { }` block nested inside a `server` block.

### Fields

| Field | Type | Default | Purpose |
|---|---|---|---|
| `path` | `string` | `""` | The URI prefix this location matches (e.g., `"/api"`, `"/cgi-bin"`) |
| `root` | `string` | `""` | Overrides the server-level root for this location |
| `index` | `string` | `""` | Overrides the server-level index for this location |
| `redirect` | `map<int, string>` | empty | A single redirect: maps HTTP code (301/302) to target URL |
| `methods` | `map<string, bool>` | empty | Allowed HTTP methods; `{"GET": true, "POST": true}` means GET and POST are allowed |
| `autoindex` | `bool` | `false` | If `true`, generate a directory listing when no index file is found |
| `upload_pass` | `string` | `""` | Path to the directory where uploaded files are saved |
| `cgi_pass` | `map<string, string>` | empty | Maps file extensions to CGI interpreter paths (e.g., `{".py" → "/usr/bin/python3"}`) |

### Constructor
The default constructor sets all strings to empty and `autoindex = false`.
