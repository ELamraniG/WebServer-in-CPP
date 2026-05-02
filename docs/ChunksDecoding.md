# ChunksDecoding

## Overview

`ChunksDecoding` implements the **HTTP chunked transfer-encoding decoder**. When a client sends a POST request with `Transfer-Encoding: chunked`, the body is split into variable-size chunks, each prefixed by its size in hexadecimal. `ChunksDecoding::decode()` reassembles those chunks into the original, continuous body.

**Problem it solves:** Chunked encoding is used when the sender doesn't know the total content length in advance (e.g., streaming). The server must understand the framing format to extract the actual data. Additionally, each call to `decode()` may only receive part of the chunks (because data arrives over the network incrementally), so the decoder must handle partial input without failing.

---

## Enum: `DecodeResult`

| Value | Meaning |
|---|---|
| `DECODE_COMPLETE` | All chunks successfully decoded; body is complete |
| `DECODE_NEED_MORE` | Not enough data yet; caller should wait for more network data |
| `DECODE_ERROR` | The chunk data is malformed |

---

## Key Members

| Member | Type | Purpose |
|---|---|---|
| `completed` | `bool` | Set to `true` once the terminal zero-size chunk is found |

---

## Public Functions

### `decode(const string& chunkedData, string& decodedBody)` → `DecodeResult`
**Purpose:** Parses the full chunked body and appends all decoded chunk data to `decodedBody`.

**The Chunked Format:**
```
<hex-size>\r\n
<data of that size>\r\n
<hex-size>\r\n
<data of that size>\r\n
0\r\n
\r\n          ← optional trailers then empty line to signal end
```

**How it works (step by step):**

1. Starts at `pos = 0` and enters a loop over `chunkedData`.
2. **Find chunk size line:** Searches for `\r\n` at `pos`. If not found, returns `DECODE_NEED_MORE`.
3. **Extract hex string:** Gets the substring `chunkedData[pos..crlfPos]`. Returns `DECODE_ERROR` if empty.
4. **Strip chunk extensions:** If a `;` exists in the hex string (chunk extension), removes everything from `;` onward.
5. **Validate hex:** Returns `DECODE_ERROR` if the string contains any non-hex characters.
6. **Parse hex size:** Calls `std::strtoul(..., 16)`. Returns `DECODE_ERROR` on parse failure.
7. **Advance pos past `\r\n`:** `pos = crlfPos + 2`.
8. **Zero-size chunk (terminal):**
   - Checks that at least 2 bytes remain (`\r\n` of the end marker).
   - If optional trailers are present (characters before the final empty line), skips them line by line until an empty `\r\n` line is found.
   - Sets `completed = true` and returns `DECODE_COMPLETE`.
9. **Data chunk:**
   - Checks that `pos + chunkSize + 2` bytes are available (data + trailing `\r\n`). Returns `DECODE_NEED_MORE` if not.
   - Appends `chunkSize` bytes to `decodedBody`.
   - Advances `pos` past the data.
   - Validates the trailing `\r\n`. Returns `DECODE_ERROR` if missing.
   - Advances `pos` past the `\r\n`.
10. If the loop ends without finding the terminal chunk, returns `DECODE_NEED_MORE`.

---

### `isComplete() const` → `bool`
**Purpose:** Returns whether the terminal zero-size chunk has been found and decoding is complete. Used by `RequestParser` to verify the full body has arrived.
