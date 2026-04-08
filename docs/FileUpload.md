# FileUpload

## What Problem Does It Solve?

When a client submits a form with `<input type="file">`, the browser encodes
the files inside the HTTP body using a format called **multipart/form-data**.
This format is **not** simple — files are wrapped in boundaries, each with
its own mini-headers.

`FileUpload` solves two problems:
1. **Parsing** — Extract individual files from the multipart body.
2. **Saving** — Write each extracted file safely to disk.

---

## What Does a Multipart Body Look Like?

When a browser uploads two files, the raw body looks like this:

```
--WebKitFormBoundaryABC123\r\n
Content-Disposition: form-data; name="file1"; filename="photo.jpg"\r\n
Content-Type: image/jpeg\r\n
\r\n
<raw binary data of photo.jpg>\r\n
--WebKitFormBoundaryABC123\r\n
Content-Disposition: form-data; name="file2"; filename="doc.pdf"\r\n
Content-Type: application/pdf\r\n
\r\n
<raw binary data of doc.pdf>\r\n
--WebKitFormBoundaryABC123--\r\n           ← closing boundary (ends with --)
```

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

### 2. Split the Body into Parts

```
Body = "--boundary\r\n<part1>\r\n--boundary\r\n<part2>\r\n--boundary--"

Step 1:  Find first "--boundary" → skip it
Step 2:  Find next "--boundary"  → everything between is Part 1
Step 3:  Find next "--boundary"  → everything between is Part 2
Step 4:  Next boundary ends with "--" → stop
```

For each part, the trailing `\r\n` before the next boundary is stripped
(it belongs to the boundary separator, not the file data).

### 3. Parse Each Part

Each part is split at `\r\n\r\n` (or `\n\n`) into:
- **Header block** — parsed into a map of lowercase key → value
- **Data block** — the raw file content

From the headers, we extract:
- `filename` from `Content-Disposition` (e.g. `"photo.jpg"`)
- `Content-Type` (e.g. `"image/jpeg"`, defaults to `application/octet-stream`)

Only parts that **have a filename** are treated as file uploads. Form fields
without a filename are silently skipped.

### 4. Result

The parsed files are returned as a `vector<FileData>`:

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
           ├── parseTheThing(request, files)
           │   ├── extractBoundary()     ← from Content-Type header
           │   ├── split body at boundaries
           │   ├── parse part headers
           │   └── extract filename + data → FileData vector
           │
           └── saveTheThing(file, uploadDir)
               ├── sanitise filename
               ├── handle duplicates
               └── write to disk
```
