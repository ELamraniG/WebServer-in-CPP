# CGIHandler

## Overview

`CGIHandler` is responsible for executing external CGI scripts (Python, PHP, Bash, etc.) in response to HTTP requests. It acts as a bridge between the web server and the CGI script process, managing the full lifecycle: spawning a child process via `fork()`/`execve()`, feeding the request body to the script via a pipe, reading the script's output via another pipe, and reporting the result (or any error) back to the `EventLoop`.

**Problem it solves:** HTTP servers need a way to run dynamic server-side code without being coupled to any specific language. CGI (Common Gateway Interface) is the standard mechanism: the server sets environment variables describing the request, the script reads from `stdin` (for POST bodies) and writes to `stdout` (its response headers + body). `CGIHandler` implements this contract in a fully non-blocking way so that the main event loop is never stalled waiting for a script to finish.

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `_pipeIn[2]` | `int[2]` | Pipe used to write the POST body **to** the child process (its stdin) |
| `_pipeOut[2]` | `int[2]` | Pipe used to read the script's output **from** the child process (its stdout) |
| `_pid` | `pid_t` | PID of the forked child process |
| `_done` | `bool` | `true` once the child has closed its stdout (EOF) |
| `_error` | `bool` | `true` if the child process exited non-zero or a pipe error occurred |
| `_code` | `int` | HTTP status code override (used by the event loop for timeouts) |
| `_output` | `string` | Accumulated output read from the CGI script |
| `_scriptPath` | `string` | Filesystem path to the CGI script |
| `_interpreter` | `string` | Interpreter binary (e.g., `/usr/bin/python3`); empty if script is directly executable |
| `_envStrings` / `_envp` | `vector` | The CGI environment variables as null-terminated `char*` array for `execve()` |

---

## Public Functions

### `CGIHandler(path, interpreter, method, queryString, body, headers)` — Constructor
**Purpose:** Initializes all state and pre-builds the CGI environment.

**How it works:**
1. Stores all parameters (script path, interpreter, HTTP method, query string, request body, and all request headers).
2. Immediately calls `buildEnv()` to convert the parameters into the standard CGI environment variable format.
3. Initializes all pipe file descriptors to `-1` (closed/invalid state).

---

### `start()` → `HttpStatus`
**Purpose:** Launches the CGI script as a child process. This is the main entry point called by `EventLoop`.

**Problem:** Running a CGI script requires forking a child process, setting up pipes for I/O, and using `execve()` to replace the child process image. Any failure in this chain (file not found, permission denied, pipe failure, fork failure) must be caught and reported as an appropriate HTTP error code so the server can respond to the client.

**How it works:**
1. Calls `access(_scriptPath, F_OK)` — returns `HTTP_NOT_FOUND` (404) if the script file doesn't exist.
2. Calls `access(_scriptPath, X_OK)` — returns `HTTP_FORBIDDEN` (403) if the script is not executable.
3. Calls `openPipes()` to create the communication pipes.
4. Calls `fork()` — returns `HTTP_INTERNAL_SERVER_ERROR` (500) on failure.
5. In the **child process**: calls `runChild()`.
6. In the **parent process**: calls `runParent()`.
7. Returns `HTTP_OK` (200) to signal a successful launch to the event loop (the actual CGI response comes later via `readOutput()`).

---

### `buildEnv()` — Private
**Purpose:** Translates the HTTP request into the CGI environment variables that the script will read.

**Problem:** CGI scripts don't receive the request as function arguments — they read environment variables. Every standard CGI variable (`REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_TYPE`, `CONTENT_LENGTH`, etc.) must be set exactly right, and all other HTTP headers must be converted to `HTTP_*` format (e.g., `Cookie` becomes `HTTP_COOKIE`).

**How it works:**
1. Pushes standard variables: `REQUEST_METHOD`, `QUERY_STRING`, `SCRIPT_FILENAME`, `SCRIPT_NAME`, `SERVER_PROTOCOL`, `GATEWAY_INTERFACE`, `REDIRECT_STATUS`, `PATH`.
2. For POST requests: also pushes `CONTENT_TYPE` and `CONTENT_LENGTH` from the corresponding headers.
3. Iterates over all remaining headers, converting each key to uppercase, replacing `-` with `_`, and prefixing with `HTTP_` (e.g., `cookie` → `HTTP_COOKIE`).
4. Builds a `char*` array (`_envp`) terminated by `NULL` for use with `execve()`.

---

### `runChild()` — Private
**Purpose:** Executed by the child process after `fork()`. Sets up the child's I/O and replaces the process with the CGI script.

**How it works:**
1. Closes the write-end of `_pipeOut` and the read-end of `_pipeIn` (child doesn't need these).
2. Uses `dup2(_pipeOut[1], STDOUT_FILENO)` to redirect the child's stdout to the pipe (so `print()` in the script goes to the server).
3. For POST: uses `dup2(_pipeIn[0], STDIN_FILENO)` to redirect the child's stdin from the pipe (so the script can `read()` the body).
4. Closes the now-redundant original pipe ends.
5. Builds the `argv` array: if an interpreter is set, it goes first (`[interpreter, scriptPath, NULL]`), otherwise just `[scriptPath, NULL]`.
6. Calls `execve()` to replace the child process with the CGI script. If `execve()` fails, exits with code 1.

---

### `runParent()` — Private
**Purpose:** Executed by the parent (server) process after `fork()`. Closes child-side pipe ends and starts writing the POST body.

**How it works:**
1. Closes `_pipeOut[1]` (the write-end of the output pipe; only the child writes to it).
2. For POST requests: closes `_pipeIn[0]` (the read-end of the input pipe; only the child reads it), then calls `writeBody()` to send the first chunk of the request body.

---

### `writeBody()` — Public
**Purpose:** Non-blocking write of the POST request body to the CGI script's stdin pipe. Called repeatedly by the `EventLoop` until the body is fully written.

**Problem:** The pipe's kernel buffer is finite (~64KB). Writing the entire body in one `write()` call would block if the buffer is full, stalling the whole server. Instead, we write what fits and return, letting the event loop call us again when the pipe is ready.

**How it works:**
1. Returns immediately if there is no body or the pipe is already closed.
2. Calls `write(_pipeIn[1], _body.c_str(), _body.size())` — a non-blocking write.
3. If `write()` returns a negative value: marks `_error = true` and closes the pipe.
4. If successful: erases the written bytes from `_body` with `_body.erase(0, bytes)`.
5. When `_body` becomes empty: closes the write-end of the pipe, signaling EOF to the script.

---

### `readOutput()` — Public
**Purpose:** Non-blocking read of the CGI script's stdout. Called by the `EventLoop` whenever the read-end of `_pipeOut` is ready (POLLIN event).

**How it works:**
1. Calls `read(_pipeOut[0], buffer, BUFFER_SIZE)` — reads up to 4096 bytes at a time.
2. If `bytes > 0`: appends the data to `_output`.
3. If `bytes == 0` (EOF): the script has finished writing. Sets `_done = true`, closes `_pipeOut[0]`, and calls `checkExitStatus()` to reap the child.
4. If `bytes < 0` (error): closes `_pipeOut[0]` and sets `_error = true`.

---

### `checkExitStatus()` — Private
**Purpose:** Checks whether the child process has exited and if it exited with an error code.

**How it works:**
Calls `waitpid(_pid, &status, WNOHANG)` (non-blocking). If the child has exited and `WEXITSTATUS(status) != 0`, sets `_error = true` (the script returned a failure exit code).

---

### `cleanup()` — Public
**Purpose:** Forcefully kills the CGI process and closes all pipes. Called by `EventLoop` when a CGI timeout is detected.

**How it works:**
1. If the child (`_pid > 0`) is still running: sends `SIGKILL` and then calls `waitpid()` to reap the zombie.
2. Calls `closeAllPipes()` to close all four pipe file descriptors.

---

### Getters: `getReadFd()`, `getWriteFd()`, `getCode()`, `getOutput()`, `isDone()`, `isError()`, `isWriteBodyDone()`
**Purpose:** Allow the `EventLoop` to inspect the state of the CGI handler and register the right file descriptors with `poll()`.

- `getReadFd()` → `_pipeOut[0]`: the fd the event loop should watch with `POLLIN`.
- `getWriteFd()` → `_pipeIn[1]`: the fd the event loop should watch with `POLLOUT` (for POST bodies).
- `isDone()` → `true` when `readOutput()` has received EOF from the script.
- `isError()` → `true` when any error occurred.
- `isWriteBodyDone()` → `true` when `_body` is fully written to the pipe.
