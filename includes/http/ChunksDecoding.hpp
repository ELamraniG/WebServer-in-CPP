#pragma once
#include <string>
#include <cstdlib>
#include <sstream>
class ChunksDecoding {
public:
  ChunksDecoding();

  bool decode(const std::string &chunkedData, std::string &decodedBody);
  bool isComplete() const;
  void reset();

private:
  bool completed;
};

