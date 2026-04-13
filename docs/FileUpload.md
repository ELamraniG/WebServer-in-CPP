# FileUpload

## What Problem Does It Solve?

When a client submits a form with `<input type="file">`, the browser encodes
the files inside the HTTP body using a format called **multipart/form-data**.
This format is **not** simple — files are wrapped in boundaries, each with
its own mini-headers.

`FileUpload` solves two problems:
1. **Parsing** — Extract one uploaded file part from the multipart body.
2. **Saving** — Write that extracted file safely to disk.

---

## What Does a Multipart Body Look Like?

When a browser uploads two files, the raw body looks like this:

```
--WebKitFormBoundaryABC123\r\n
Content-Disposition: form-data; name="file"; filename="photo.jpg"\r\n
Content-Type: image/jpeg\r\n
\r\n
<raw binary data of photo.jpg>\r\n
--WebKitFormBoundaryABC123--\r\n

### Key elements:

| Element                  | Description                                             |
|--------------------------|---------------------------------------------------------|
| **Boundary**             | A unique string that separates parts (from `Content-Type` header) |
| **`--` prefix**          | Every boundary in the body is prefixed with `--`        |
| **Part headers**         | Each part has its own `Content-Disposition` and optional `Content-Type` |
| **`filename="..."`**     | The original filename from the client's filesystem      |
| **Blank line `\r\n\r\n`**| Separates part headers from part data                   |
| **`--boundary--`**       | The closing boundary with trailing `--` = no more parts |

---

## How Parsing Works — Step by Step

### 1. Extract the Boundary

The boundary comes from the request's `Content-Type` header:

```
Content-Type: multipart/form-data; boundary=WebKitFormBoundaryABC123
```

`extractBoundary()` finds `boundary=` and extracts the value, trimming
quotes and spaces.

### 2. Find the First Multipart Part

```
Body = "--boundary\r\n<part1>\r\n--boundary\r\n<part2>\r\n--boundary--"

Step 1:  Find first "--boundary" → skip it
Step 2:  Find next "--boundary"  → everything between is the part to parse
Step 3:  Trim trailing CRLF before that boundary
```

Current implementation parses only this first part.

### 3. Parse That Part

Each part is split at `\r\n\r\n` (or `\n\n`) into:
- **Header block** — parsed into a map of lowercase key → value
- **Data block** — the raw file content

From the headers, we extract:
- `filename` from `Content-Disposition` (e.g. `"photo.jpg"`)
- `Content-Type` (e.g. `"image/jpeg"`, defaults to `application/octet-stream`)

The part must contain a `filename` attribute in `Content-Disposition`.
If filename is missing, parsing fails.

### 4. Result

The parsed file is returned through a single `FileData` output argument:

```cpp
struct FileData {
    std::string filename;      // "photo.jpg"
    std::string contentType;   // "image/jpeg"
    std::string data;          // raw binary content
};
```

---

## How Saving Works

`saveTheThing()` writes a single `FileData` to disk:

```
1. VALIDATE FILENAME
   ├─ Empty filename? → reject
   └─ Filename ok

2. VALIDATE UPLOAD DIRECTORY
   ├─ Directory doesn't exist? → reject
   ├─ Path is not a directory? → reject
   └─ Directory ok

3. SANITISE FILENAME
   ├─ Strip path components (remove everything before last / or \)
   │   "../../evil.sh" → "evil.sh"
   ├─ Reject null bytes and control characters (< 0x20 or 0x7F)
   └─ Replace "." or ".." with "upload"

4. BUILD FULL PATH
   uploadDir + "/" + safeName

5. HANDLE DUPLICATES
   If file already exists, try appending _1, _2, ... _9999
   "photo.jpg" → "photo_1.jpg" → "photo_2.jpg" → ...

6. WRITE FILE
   Open in binary mode (ios::binary | ios::trunc)
   Write raw data
   Verify write succeeded
```

---

## Security Measures

| Threat                          | Protection                                          |
|---------------------------------|-----------------------------------------------------|
| Path traversal (`../../etc/passwd`) | Strip path components, keep only basename          |
| Null byte injection             | Reject any character < 0x20 or == 0x7F              |
| Filename "." or ".."            | Replace with "upload"                               |
| Overwriting existing files      | Auto-generate unique name with `_N` suffix          |
| Upload to non-existent dir      | Validate directory exists before writing            |
| Arbitrary large uploads         | Enforced by MethodHandler via `maxBodySize` check   |

---

## Relationship With Other Classes

```
           MethodHandler (POST handler)
                │
                │  Content-Type contains "multipart/form-data"
                │
                ▼
           FileUpload
           ├── parseTheThing(request, file)
           │   ├── extractBoundary()     ← from Content-Type header
           │   ├── locate first part between boundaries
           │   ├── parse part headers
           │   └── extract filename + data → single FileData
           │
           └── saveTheThing(file, uploadDir)
               ├── sanitise filename
               ├── handle duplicates
               └── write to disk
```
