# RouteConfig

## What Problem Does It Solve?

A web server doesn't treat every URL the same way. For example:
- `/api/` might only allow POST requests
- `/uploads/` needs to know *where* to save files on disk
- `/cgi-bin/` should execute scripts through PHP
- `/docs/` should show a directory listing if no index file exists

All these per-route settings need to live somewhere. `RouteConfig` is the
**configuration container** for a single route — it holds every setting that
controls how the server handles requests matching that route.

> **Note:** This is currently a **temporary/stub** class. Person 3 on the team
> will replace it with a real implementation that reads these values from the
> server configuration file.

---

## Configuration Fields

| Field            | Type                     | What it controls                                    | Default  |
|------------------|--------------------------|-----------------------------------------------------|----------|
| `root`           | `std::string`            | Document root — base directory on disk               | `""`     |
| `allowedMethods` | `std::vector<string>`    | Which HTTP methods are permitted (empty = all)       | empty    |
| `uploadStore`    | `std::string`            | Directory where uploaded files are saved             | `""`     |
| `cgiPass`        | `std::string`            | Path to CGI interpreter (e.g. `/usr/bin/php`)        | `""`     |
| `redirect`       | `std::string`            | URL to redirect to (301 Moved Permanently)           | `""`     |
| `maxBodySize`    | `size_t`                 | Maximum allowed request body size in bytes (0 = unlimited) | `0`  |
| `index`          | `std::string`            | Default index file (e.g. `"index.html"`)             | `""`     |
| `autoindex`      | `bool`                   | Whether to show a directory listing                  | `false`  |

---

## How Each Field Is Used

### `root` — Document Root
The base path on the filesystem. The URI is appended to this to find files.
```
root = "/var/www/html"
URI  = "/images/cat.jpg"
File = "/var/www/html/images/cat.jpg"
```

### `allowedMethods` — Method Filtering
If non-empty, only the listed methods are allowed. A request with any other
method gets a **405 Method Not Allowed**.
```
allowedMethods = ["GET", "POST"]
→ DELETE request → 405
```

### `uploadStore` — File Upload Directory
Tells `FileUpload` where to save files on disk. Must point to an existing
directory with write permissions.

### `cgiPass` — CGI Interpreter
Path to the executable that runs CGI scripts.
```
cgiPass = "/usr/bin/php-cgi"
→ .php files under this route will be executed through php-cgi
```

### `redirect` — HTTP Redirect
If set, the server immediately responds with a **301 Moved Permanently** and
the `Location` header set to this value. No file serving happens.

### `maxBodySize` — Body Size Limit
Maximum number of bytes allowed in the request body. If exceeded, the server
returns **413 Payload Too Large**. A value of 0 means no limit.

### `index` — Default Index File
When a directory is requested, the server looks for this file inside it
first. If found, it serves that file instead of showing the directory.
```
index = "index.html"
Request: GET /docs/
Tries:   /var/www/html/docs/index.html
```

### `autoindex` — Directory Listing
If `true` and no index file is found, the server generates an HTML page
listing all files and subdirectories. If `false`, it returns **403 Forbidden**.

---

## How It Connects to Other Classes

```
   Config File (parsed by Person 3)
          │
          │  fills in values
          ▼
     RouteConfig
          │
          ├──────────────► MethodHandler
          │                 reads: root, allowedMethods, redirect,
          │                        maxBodySize, index, autoindex
          │
          ├──────────────► CGIHandler
          │                 reads: cgiPass, root
          │
          └──────────────► FileUpload (via MethodHandler)
                            reads: uploadStore
```

---

## Example Configuration (what Person 3 will parse)

```nginx
location /uploads {
    root         /var/www/data;
    allowed_methods GET POST;
    upload_store /var/www/data/uploads;
    client_max_body_size 10M;
}

location /cgi-bin {
    root         /var/www/cgi;
    cgi_pass     /usr/bin/php-cgi;
}

location /docs {
    root         /var/www/html;
    index        index.html;
    autoindex    on;
}
```

Each `location` block maps to one `RouteConfig` object.
