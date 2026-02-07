#include "../includes/http/ChunksDecoding.hpp"
#include <cassert>
#include <iostream>
#include <string>

static int passed = 0;
static int failed = 0;

#define TEST(name)                                                             \
  do {                                                                         \
    std::cout << "  [TEST] " << name << " ... ";                               \
  } while (0)

#define PASS()                                                                 \
  do {                                                                         \
    std::cout << "PASS" << std::endl;                                          \
    ++passed;                                                                  \
  } while (0)

#define FAIL(msg)                                                              \
  do {                                                                         \
    std::cout << "FAIL: " << msg << std::endl;                                 \
    ++failed;                                                                  \
  } while (0)

#define EXPECT_EQ(a, b, msg)                                                   \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      FAIL(msg);                                                               \
      return;                                                                  \
    }                                                                          \
  } while (0)

// ============================================================================
// 1. Basic valid chunked bodies
// ============================================================================

static void test_single_chunk() {
  TEST("Single chunk");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5\r\nhello\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, "hello", "body mismatch");
  EXPECT_EQ(decoder.isComplete(), true, "should be complete");
  PASS();
}

static void test_multiple_chunks() {
  TEST("Multiple chunks");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "9\r\nuser=john\r\n7\r\n&age=30\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, "user=john&age=30", "body mismatch");
  EXPECT_EQ(decoder.isComplete(), true, "should be complete");
  PASS();
}

static void test_single_byte_chunks() {
  TEST("Single byte chunks");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "1\r\nA\r\n1\r\nB\r\n1\r\nC\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, "ABC", "body mismatch");
  PASS();
}

static void test_large_hex_chunk_size() {
  TEST("Large hex chunk size (uppercase)");
  ChunksDecoding decoder;
  std::string body;
  // 1A = 26 bytes
  std::string data = "abcdefghijklmnopqrstuvwxyz";
  std::string input = "1A\r\n" + data + "\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, data, "body mismatch");
  PASS();
}

static void test_lowercase_hex() {
  TEST("Lowercase hex chunk size");
  ChunksDecoding decoder;
  std::string body;
  // 1a = 26 bytes
  std::string data = "abcdefghijklmnopqrstuvwxyz";
  std::string input = "1a\r\n" + data + "\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, data, "body mismatch");
  PASS();
}

static void test_mixed_case_hex() {
  TEST("Mixed case hex chunk size");
  ChunksDecoding decoder;
  std::string body;
  // aB = 171 bytes
  std::string data(171, 'X');
  std::string input = "aB\r\n" + data + "\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, data, "body mismatch");
  PASS();
}

static void test_zero_chunk_only() {
  TEST("Zero chunk only (empty body)");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, "", "body should be empty");
  EXPECT_EQ(decoder.isComplete(), true, "should be complete");
  PASS();
}

// ============================================================================
// 2. Incremental / streaming (DECODE_NEED_MORE)
// ============================================================================

static void test_incomplete_chunk_size_line() {
  TEST("Incomplete chunk size line (no CRLF)");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_NEED_MORE, "expected DECODE_NEED_MORE");
  PASS();
}

static void test_incomplete_chunk_size_partial_crlf() {
  TEST("Partial CRLF after chunk size");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5\r";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_NEED_MORE, "expected DECODE_NEED_MORE");
  PASS();
}

static void test_incomplete_chunk_data() {
  TEST("Incomplete chunk data");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5\r\nhel";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_NEED_MORE, "expected DECODE_NEED_MORE");
  EXPECT_EQ(body, "", "body should be empty (not appended yet)");
  PASS();
}

static void test_incomplete_chunk_data_missing_trailing_crlf() {
  TEST("Chunk data complete but missing trailing CRLF");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5\r\nhello";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_NEED_MORE, "expected DECODE_NEED_MORE");
  PASS();
}

static void test_incomplete_final_crlf() {
  TEST("Missing final CRLF after zero chunk");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5\r\nhello\r\n0\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_NEED_MORE, "expected DECODE_NEED_MORE");
  PASS();
}

static void test_empty_input() {
  TEST("Empty input");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_NEED_MORE, "expected DECODE_NEED_MORE");
  PASS();
}

static void test_incremental_calls_no_duplicates() {
  TEST("Incremental calls produce no duplicate data");
  ChunksDecoding decoder;
  std::string body;

  // Call 1: incomplete
  std::string buf = "5\r\nhello\r\n";
  DecodeResult res = decoder.decode(buf, body);
  EXPECT_EQ(res, DECODE_NEED_MORE, "call 1: expected DECODE_NEED_MORE");

  // Call 2: append more data and retry with full buffer
  buf += "0\r\n\r\n";
  res = decoder.decode(buf, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "call 2: expected DECODE_COMPLETE");
  EXPECT_EQ(body, "hello", "body should be 'hello', not 'hellohello'");
  PASS();
}

static void test_incremental_multi_chunk_no_duplicates() {
  TEST("Incremental multi-chunk no duplicates");
  ChunksDecoding decoder;
  std::string body;

  // Call 1: first chunk complete, second incomplete
  std::string buf = "9\r\nuser=john\r\n7\r\n";
  DecodeResult res = decoder.decode(buf, body);
  EXPECT_EQ(res, DECODE_NEED_MORE, "call 1: expected DECODE_NEED_MORE");

  // Call 2: full buffer
  buf += "&age=30\r\n0\r\n\r\n";
  res = decoder.decode(buf, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "call 2: expected DECODE_COMPLETE");
  EXPECT_EQ(body, "user=john&age=30", "no duplicate data");
  PASS();
}

// ============================================================================
// 3. Malformed input (DECODE_ERROR)
// ============================================================================

static void test_invalid_hex_char() {
  TEST("Invalid hex character in chunk size");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5G\r\nhello\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_ERROR, "expected DECODE_ERROR");
  PASS();
}

static void test_non_hex_chunk_size() {
  TEST("Non-hex chunk size (letters only)");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "ZZ\r\ndata\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_ERROR, "expected DECODE_ERROR");
  PASS();
}

static void test_malformed_crlf_after_chunk_data() {
  TEST("Malformed CRLF after chunk data");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5\r\nhelloXX0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_ERROR, "expected DECODE_ERROR");
  PASS();
}

static void test_empty_hex_field() {
  TEST("Empty hex field (consecutive CRLFs)");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_ERROR, "expected DECODE_ERROR");
  PASS();
}

// ============================================================================
// 4. Chunk extensions
// ============================================================================

static void test_chunk_extension_stripped() {
  TEST("Chunk extension is stripped");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5;ext=val\r\nhello\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, "hello", "body mismatch");
  PASS();
}

static void test_chunk_extension_multiple() {
  TEST("Multiple chunk extensions stripped");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5;ext1=a;ext2=b\r\nhello\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, "hello", "body mismatch");
  PASS();
}

static void test_zero_chunk_with_extension() {
  TEST("Zero chunk with extension");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5\r\nhello\r\n0;ext=done\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, "hello", "body mismatch");
  PASS();
}

// ============================================================================
// 5. Trailer headers
// ============================================================================

static void test_single_trailer() {
  TEST("Single trailer header");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5\r\nhello\r\n0\r\nTrailer: value\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, "hello", "body mismatch");
  PASS();
}

static void test_multiple_trailers() {
  TEST("Multiple trailer headers");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5\r\nhello\r\n0\r\nX-Check: abc\r\nX-Time: 123\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, "hello", "body mismatch");
  PASS();
}

static void test_incomplete_trailer() {
  TEST("Incomplete trailer (no final CRLF)");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "5\r\nhello\r\n0\r\nTrailer: value\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_NEED_MORE, "expected DECODE_NEED_MORE");
  PASS();
}

// ============================================================================
// 6. Reset functionality
// ============================================================================

static void test_reset() {
  TEST("Reset and reuse decoder");
  ChunksDecoding decoder;
  std::string body;

  // First decode
  std::string input1 = "5\r\nhello\r\n0\r\n\r\n";
  DecodeResult res = decoder.decode(input1, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "first decode: expected DECODE_COMPLETE");
  EXPECT_EQ(decoder.isComplete(), true, "should be complete");

  // Reset
  decoder.reset();
  EXPECT_EQ(decoder.isComplete(), false, "should not be complete after reset");

  // Second decode
  std::string input2 = "5\r\nworld\r\n0\r\n\r\n";
  res = decoder.decode(input2, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "second decode: expected DECODE_COMPLETE");
  EXPECT_EQ(body, "world", "body should be 'world'");
  PASS();
}

// ============================================================================
// 7. Edge cases with data content
// ============================================================================

static void test_data_contains_crlf_pattern() {
  TEST("Chunk data contains CRLF-like bytes");
  ChunksDecoding decoder;
  std::string body;
  // 11 = 17 bytes, data contains \r\n inside
  std::string input = "11\r\nline1\r\nline2\r\nend\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, "line1\r\nline2\r\nend",
            "body should preserve internal CRLFs");
  PASS();
}

static void test_data_contains_zero_crlf_pattern() {
  TEST("Chunk data contains 0\\r\\n\\r\\n pattern");
  ChunksDecoding decoder;
  std::string body;
  // A = 10 bytes
  std::string input = "A\r\n0\r\n\r\nhello\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, "0\r\n\r\nhello", "body should contain the fake terminator");
  PASS();
}

static void test_binary_data_in_chunk() {
  TEST("Binary data (null bytes) in chunk");
  ChunksDecoding decoder;
  std::string body;
  std::string data;
  data += '\0';
  data += '\x01';
  data += '\xFF';
  data += '\0';
  data += 'A';
  // 5 bytes
  std::string input = "5\r\n" + data + "\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body.size(), (size_t)5, "body should be 5 bytes");
  EXPECT_EQ(body, data, "body should match binary data");
  PASS();
}

// ============================================================================
// 8. Leading zeros in hex
// ============================================================================

static void test_leading_zeros_in_hex() {
  TEST("Leading zeros in hex chunk size");
  ChunksDecoding decoder;
  std::string body;
  std::string input = "005\r\nhello\r\n0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, "hello", "body mismatch");
  PASS();
}

// ============================================================================
// 9. Many chunks
// ============================================================================

static void test_many_small_chunks() {
  TEST("Many small chunks");
  ChunksDecoding decoder;
  std::string body;
  std::string input;
  std::string expected;

  for (int i = 0; i < 100; ++i) {
    input += "1\r\n";
    input += (char)('A' + (i % 26));
    input += "\r\n";
    expected += (char)('A' + (i % 26));
  }
  input += "0\r\n\r\n";

  DecodeResult res = decoder.decode(input, body);
  EXPECT_EQ(res, DECODE_COMPLETE, "expected DECODE_COMPLETE");
  EXPECT_EQ(body, expected, "body mismatch");
  EXPECT_EQ(body.size(), (size_t)100, "should be 100 bytes");
  PASS();
}

// ============================================================================
// 10. isComplete before decode
// ============================================================================

static void test_is_complete_before_decode() {
  TEST("isComplete is false before any decode");
  ChunksDecoding decoder;
  EXPECT_EQ(decoder.isComplete(), false, "should be false initially");
  PASS();
}

// ============================================================================
// Main
// ============================================================================

int main() {
  std::cout << "=============================" << std::endl;
  std::cout << " ChunksDecoding Test Suite" << std::endl;
  std::cout << "=============================" << std::endl;

  // 1. Basic valid
  test_single_chunk();
  test_multiple_chunks();
  test_single_byte_chunks();
  test_large_hex_chunk_size();
  test_lowercase_hex();
  test_mixed_case_hex();
  test_zero_chunk_only();

  // 2. Incremental / streaming
  test_incomplete_chunk_size_line();
  test_incomplete_chunk_size_partial_crlf();
  test_incomplete_chunk_data();
  test_incomplete_chunk_data_missing_trailing_crlf();
  test_incomplete_final_crlf();
  test_empty_input();
  test_incremental_calls_no_duplicates();
  test_incremental_multi_chunk_no_duplicates();

  // 3. Malformed input
  test_invalid_hex_char();
  test_non_hex_chunk_size();
  test_malformed_crlf_after_chunk_data();
  test_empty_hex_field();

  // 4. Chunk extensions
  test_chunk_extension_stripped();
  test_chunk_extension_multiple();
  test_zero_chunk_with_extension();

  // 5. Trailers
  test_single_trailer();
  test_multiple_trailers();
  test_incomplete_trailer();

  // 6. Reset
  test_reset();

  // 7. Edge cases
  test_data_contains_crlf_pattern();
  test_data_contains_zero_crlf_pattern();
  test_binary_data_in_chunk();

  // 8. Leading zeros
  test_leading_zeros_in_hex();

  // 9. Many chunks
  test_many_small_chunks();

  // 10. Initial state
  test_is_complete_before_decode();

  std::cout << "=============================" << std::endl;
  std::cout << " Results: " << passed << " passed, " << failed << " failed"
            << std::endl;
  std::cout << "=============================" << std::endl;

  return (failed > 0) ? 1 : 0;
}
