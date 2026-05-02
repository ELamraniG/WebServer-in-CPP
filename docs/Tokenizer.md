# Tokenizer

## Overview

`Tokenizer` is the first stage of the configuration file pipeline. It reads a `.conf` file from disk, strips comments and blank lines, and splits the text into a flat list of meaningful tokens (words, `{`, `}`, `;`). This token list is then handed to `Parser` for semantic analysis.

**Problem it solves:** Raw configuration text is not easy to parse directly — it has whitespace, newlines, `#` comments, and special punctuation characters mixed together. `Tokenizer` abstracts all of that away, giving the `Parser` a clean, simple sequence of strings to work with.

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `tokens` | `vector<string>` | The flat, ordered list of tokens produced from the config file |

---

## Public Functions

### `Tokenizer(const char* filePath)` — Constructor
**Purpose:** Opens the config file and performs the full tokenization in one step.

**How it works:**
Immediately calls `parse(filePath)`. All work is done during construction; after the constructor returns, `tokens` is fully populated.

---

### `parse(const char* filePath)`
**Purpose:** Reads the config file line by line, strips comments and blank lines, and feeds each non-empty line to `tokenize()`.

**Problem:** Config files contain comments (starting with `#`) and blank lines that must be ignored. Reading byte-by-byte would be complex.

**How it works:**
1. Opens the file with `std::ifstream`. Throws `std::runtime_error` if the file cannot be opened.
2. Reads the file line by line with `std::getline`.
3. For each line, finds the first `#` character and truncates the string there (stripping the comment).
4. Calls `trim()` to remove leading/trailing whitespace.
5. Skips empty lines after trimming.
6. Passes the cleaned line to `tokenize()`.
7. After all lines are read, throws `std::runtime_error` if `tokens` is still empty (empty config is invalid).

---

### `tokenize(std::string s)`
**Purpose:** Splits one line of text into individual tokens and appends them to the `tokens` vector.

**Problem:** A single config line can contain multiple tokens separated by spaces, and special characters (`{`, `}`, `;`) are always tokens by themselves, even if not surrounded by spaces (e.g., `listen 8080;` → `["listen", "8080", ";"]`).

**How it works:**
Iterates character by character:
- **Whitespace:** If a `current_token` has been built up, pushes it to `tokens` and resets it.
- **Special characters** (`{`, `}`, `;`): First flushes any `current_token`, then pushes the special character itself as a single-character token.
- **All other characters:** Appends the character to `current_token`.

After the loop, any remaining `current_token` is pushed.

---

### Example

Given the config line: `root /var/www;`

The tokenizer produces: `["root", "/var/www", ";"]`

Given: `location /api {`

The tokenizer produces: `["location", "/api", "{"]`
