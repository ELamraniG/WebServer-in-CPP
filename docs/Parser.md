# Parser

## Overview

`Parser` is the second stage of the configuration pipeline. It takes the flat token list produced by `Tokenizer` and builds a structured vector of `Server_block` objects (each containing `location_block`s). It performs full semantic validation: checking for required directives, duplicate keys, unknown keywords, invalid values, and structural correctness (e.g., `location` must be inside `server`).

**Problem it solves:** Raw tokens have no inherent structure. The `Parser` enforces the grammar of the configuration file, turning a list like `["server", "{", "listen", "8080", ";", "}"]` into a proper `Server_block` with `port = "8080"`. It also catches configuration errors early (at startup), preventing runtime failures.

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `_tokens` | `vector<string>` | The token list from `Tokenizer` |
| `servers` | `vector<Server_block>` | The list of fully parsed server blocks (the output) |
| `state` | `block_type` enum | Current parser state: `GLOBAL`, `SERVER`, or `LOCATION` |
| `current_server` | `Server_block*` | Pointer to the server block currently being parsed |
| `current_location` | `location_block*` | Pointer to the location block currently being parsed |
| `_server_seen` | `set<string>` | Tracks which directives have already been seen in the current server block (prevents duplicates) |
| `_location_seen` | `set<string>` | Same, for the current location block |

---

## Public Functions

### `Parser(vector<string>& tokens)` — Constructor
**Purpose:** Initializes the parser with the token list and sets the state to `GLOBAL`.

---

### `parse()`
**Purpose:** The main parsing loop. Iterates over all tokens and builds the `servers` vector while enforcing the grammar.

**Problem:** The config has a nested block structure (`server { location { } }`). A simple linear scan needs to track context (are we inside a server block? a location block?) and validate that directives appear in the right context.

**How it works:**
1. Iterates over `_tokens` with index `i`.
2. **`server` token:** Validates that we are at `GLOBAL` scope and that the next token is `{`. Creates a new `Server_block`, pushes it to `servers`, sets `current_server`, clears `_server_seen`, and sets `state = SERVER`.
3. **`location` token:** Validates that we are in `SERVER` scope, that the next token is a path starting with `/`, and that the path is not a duplicate. Creates a new `location_block`, sets `current_location`, clears `_location_seen`, and sets `state = LOCATION`.
4. **`}` token:** Pops the current context. If in `LOCATION`, returns to `SERVER`. If in `SERVER`, validates that `listen` was provided and returns to `GLOBAL`.
5. **`;` token:** Skipped (it's a delimiter already consumed during directive parsing).
6. **Other tokens:** Dispatches to `parse_server_block(i)` or `parse_location_block(i)` depending on `state`.
7. After the loop: validates that `state` is back to `GLOBAL` (no unclosed `}`) and that at least one server was defined.
8. Checks for duplicate `host:port` pairs across all server blocks and throws if found.

---

### `parse_server_block(size_t& i)` — Private
**Purpose:** Handles one directive found inside a `server { }` block.

**Problem:** Each directive has a fixed number of arguments and must be terminated by `;`. Some directives (like `error_page`) take two arguments. Duplicates (like defining `root` twice) must be caught.

**How it works:** A large `if/else if` chain checks the current token:
- **`listen`:** Expects `listen <value> ;`. Calls `split_listen()` to parse `host:port` or just `port`. Stores in `current_server->port` and `current_server->host`. Uses `mark_seen()` to prevent duplicates.
- **`error_page`:** Expects `error_page <code> <path> ;`. Validates the error code is between 300–599. Stores in `current_server->error_pages[code]`.
- **`client_max_body_size`:** Expects `client_max_body_size <value> ;`. Calls `isvalid_client_number()` to parse values like `10M`, `1024K`. Uses `mark_seen()`.
- **`root`:** Expects `root <path> ;`. Uses `mark_seen()`.
- **`index`:** Expects `index <file> ;`. Uses `mark_seen()`.
- **Unknown token:** Throws `std::runtime_error`.

---

### `parse_location_block(size_t& i)` — Private
**Purpose:** Handles one directive found inside a `location { }` block.

**How it works:** Similar `if/else if` chain:
- **`root`**, **`index`**, **`autoindex`:** Standard directives with one argument and `;`.
- **`allowed_methods`:** Expects one or more of `GET`, `POST`, `DELETE` before `;`. Stores each as `current_location->methods[method] = true`.
- **`upload_pass`:** Stores the upload directory path.
- **`cgi_pass`:** Expects `cgi_pass <.ext> <interpreter_path> ;`. The extension must start with `.`. Stores in `current_location->cgi_pass[ext]`.
- **`return`:** Expects `return <code> <url> ;`. Code must be 300–399. Only one `return` allowed per location.
- **Unknown token:** Throws `std::runtime_error`.

---

### `getServers() const` → `vector<Server_block>`
**Purpose:** Returns the parsed server configuration after `parse()` completes successfully. Called by `main()` to hand the config to the `Server` and `EventLoop` constructors.

---

## Helper Functions (in `Parser_helper.cpp`)

| Function | Purpose |
|---|---|
| `isvalidport(string& port)` | Validates port is a numeric string in range 1–65535 |
| `isvalid_error_number(string& n)` | Validates error code is an integer 300–599, returns it or -1 |
| `isvalid_return_number(string& n)` | Validates redirect code is 300–399, returns it or -1 |
| `isvalid_client_number(string& n)` | Parses body size with optional `K`/`M`/`G` suffix, returns bytes |
