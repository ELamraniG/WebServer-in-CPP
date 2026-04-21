#pragma once
#include <cstdlib>
#include <sstream>
#include <string>

enum DecodeResult { DECODE_COMPLETE, DECODE_NEED_MORE, DECODE_ERROR };

class ChunksDecoding {
public:
  ChunksDecoding();

  DecodeResult decode(const std::string &chunkedData, std::string &decodedBody);
  bool isComplete() const;

private:
  bool completed;
};
