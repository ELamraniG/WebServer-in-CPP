# FileUpload

## Overview

`FileUpload` handles **multipart/form-data file uploads**. It parses the raw request body to extract the file's name, content type, and binary data, then writes that data to the configured upload directory on disk.

**Problem it solves:** When a browser uploads a file via an HTML `<form enctype="multipart/form-data">`, the body is a complex multi-part format with boundaries, per-part headers, and binary content. `FileUpload` knows how to decode this format and safely save the file, avoiding path traversal attacks in the process.

---

## Struct: `FileData`

A plain data holder populated by `parseTheThing()` and consumed by `saveTheThing()`.

| Field | Type | Purpose |
|---|---|---|
| `filename` | `string` | Original filename from `Content-Disposition` header |
| `contentType` | `string` | MIME type from the part's `Content-Type` header |
| `data` | `string` | The raw binary file content |

---

## Public Functions

### `parseTheThing(const HTTPRequest& request, FileData& parsedFile)` → `bool`
**Purpose:** Extracts the first file part from a `multipart/form-data` request body and populates `parsedFile`.

**Problem:** The multipart body looks like:
```
--<boundary>\r\n
Content-Disposition: form-data; name="file"; filename="photo.jpg"\r\n
Content-Type: image/jpeg\r\n
\r\n
<binary data>\r\n
--<boundary>--\r\n
```
The boundary string varies per request. The part header must be separated from the part body. The filename must be extracted from the `Content-Disposition` header.

**How it works:**
1. Gets the `Content-Type` header and calls `extractBoundary()` to get the boundary string. Returns `false` if no boundary.
2. Finds the first occurrence of `--<boundary>` in the body. Returns `false` if not found.
3. Advances `pos` past the boundary line and any `\r\n`.
4. Finds the next boundary (marks the end of the first part).
5. Trims the trailing `\r\n` before the next boundary to isolate the part content.
6. Splits the part content at `\r\n\r\n` (or `\n\n`) to separate part-headers from part-data.
7. Calls `parsePartHeaders()` to parse the part's `Content-Disposition` and `Content-Type` headers.
8. Calls `extractAttribute(disposition, "filename")` to get the filename. Returns `false` if no filename.
9. Populates `parsedFile.filename`, `parsedFile.data`, and `parsedFile.contentType`.

---

### `saveTheThing(const FileData& file, const string& uploadDir)` → `bool`
**Purpose:** Writes the file data to the upload directory on disk, safely.

**Problem:** A malicious user could send a filename like `../../etc/passwd` to write outside the upload directory. The filename must be sanitized.

**How it works:**
1. Returns `false` if `file.filename` is empty.
2. **Strips directory components:** Uses `find_last_of("/\\")` to extract only the basename from the filename, preventing path traversal.
3. **Rejects control characters:** Returns `false` if any byte in the sanitized name is a control character (< 0x20 or 0x7F).
4. **Rejects `.` and `..`:** Returns `false` if the sanitized name is `.` or `..`.
5. Builds the full path: `uploadDir + "/" + safeName`.
6. Opens the file with `std::ofstream` in binary + truncate mode.
7. Writes `file.data` in one `ofs.write()` call.
8. Returns `true` on success, `false` if the file couldn't be opened or `ofs.good()` fails.

---

## Private Functions

### `extractBoundary(const string& contentType) const` → `string`
**Purpose:** Parses the boundary string from `Content-Type: multipart/form-data; boundary=----WebKitFormBoundary...`.

**How it works:** Finds `"boundary="` in the content type string, extracts everything after it, and trims surrounding spaces and quotes.
