# ChunksDecoding

## What Problem Does It Solve?

HTTP allows a sender to transmit data in **chunks** instead of specifying the
total size upfront. This is called **chunked transfer encoding**
(`Transfer-Encoding: chunked`).

Instead of saying "here are 5000 bytes", the sender says:

```
"here are 100 bytes" … data … 
"here are 200 bytes" … data …
"here are 0 bytes"   ← done!
```

`ChunksDecoding` **reads this chunked format** and **reconstructs the
original, complete body** so the rest of the server can use it as if it were
a normal, flat body.

---

## What Does Chunked Data Look Like?

```
<chunk-size in hex>\r\n
<chunk-data (exactly chunk-size bytes)>\r\n
<chunk-size in hex>\r\n
<chunk-data>\r\n
...
0\r\n                        ← size 0 = last chunk
<optional trailers>\r\n      ← extra headers we skip
\r\n                         ← final blank line
```

### Concrete Example

```
4\r\n          ← chunk size = 4 bytes
Wiki\r\n       ← chunk data
6\r\n          ← chunk size = 6 bytes
pedia \r\n     ← chunk data (note: the space is part of the data)
0\r\n          ← last chunk (size 0)
\r\n           ← end
```

Result after decoding: `"Wikipedia "`

---

## How It Works — Step by Step

```
Input: "4\r\nWiki\r\n6\r\npedia \r\n0\r\n\r\n"

Step 1:  pos=0
         Find \r\n at pos 1 → hexStr = "4"
         Validate hex chars → ok
         Convert to number → chunkSize = 4
         pos = 3 (skip past "4\r\n")
         chunkSize ≠ 0 → read 4 bytes of data
         Append "Wiki" to decodedBody
         Check data is followed by \r\n → ok
         pos = 9

Step 2:  pos=9
         Find \r\n at pos 10 → hexStr = "6"
         Convert → chunkSize = 6
         pos = 12
         Append "pedia " to decodedBody
         pos = 20

Step 3:  pos=20
         Find \r\n → hexStr = "0"
         Convert → chunkSize = 0 → LAST CHUNK
         pos = 23
         Check for \r\n → found → DECODE_COMPLETE

decodedBody = "Wikipedia "
```

---

## Chunk Size Validation

The size line can contain optional **chunk extensions** after a `;`:

```
a;ext-name=ext-value\r\n
```

The decoder strips everything after `;` and only looks at the hex digits.

Each character is validated to be a valid hexadecimal digit (`0-9`, `a-f`,
`A-F`). If any character is invalid → `DECODE_ERROR`.

The hex string is converted to a number using `strtoul(…, 16)`. If the
conversion fails → `DECODE_ERROR`.

---

## The Three Return States

| Result              | Meaning                                                  |
|---------------------|----------------------------------------------------------|
| `DECODE_COMPLETE`   | All chunks received, including the terminating `0` chunk |
| `DECODE_NEED_MORE`  | The data is valid so far but incomplete — wait for more  |
| `DECODE_ERROR`      | Malformed chunk format (bad hex, missing `\r\n`, etc.)   |

---

## Edge Cases Handled

| Situation                            | Behaviour                                  |
|--------------------------------------|--------------------------------------------|
| Chunk size line not fully received   | `DECODE_NEED_MORE`                         |
| Chunk data not fully received        | `DECODE_NEED_MORE`                         |
| Final `0\r\n` received but no `\r\n`| `DECODE_NEED_MORE`                         |
| Optional trailer headers after `0`   | Skipped line by line until blank line      |
| Trailer not fully received           | `DECODE_NEED_MORE`                         |
| Non-hex character in size            | `DECODE_ERROR`                             |
| Missing `\r\n` after chunk data      | `DECODE_ERROR`                             |
| Empty hex string                     | `DECODE_ERROR`                             |

---

## Relationship With Other Classes

```
RequestParser
    │
    │  detects "Transfer-Encoding: chunked"
    │  calls ChunksDecoding::decode()
    │
    ├──► ChunksDecoding
    │        │
    │        └──► returns decoded body string
    │
    └──► stores body in HTTPRequest
```

---

## Public Interface

| Method                              | Purpose                                         |
|-------------------------------------|-------------------------------------------------|
| `decode(chunkedData, decodedBody)`  | Decode chunks → returns `DecodeResult`          |
| `isComplete()`                      | `true` once the `0` chunk has been processed    |
