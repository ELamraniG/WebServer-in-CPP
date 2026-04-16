#include "../../includes/http/ChunksDecoding.hpp"

#include <cerrno>

ChunksDecoding::ChunksDecoding() : completed(false) {}

DecodeResult ChunksDecoding::decode(const std::string &chunkedData,
                                    std::string &decodedBody) {
  decodedBody.clear();
  size_t pos = 0;

  while (pos < chunkedData.size()) {
    // get the pos of the for /r/n
    size_t crlfPos = chunkedData.find("\r\n", pos);
    if (crlfPos == std::string::npos) {
      return DECODE_NEED_MORE;
    }

    // get the size of the chunk in hex
    std::string hexStr = chunkedData.substr(pos, crlfPos - pos);
    if (hexStr.empty()) {
      return DECODE_ERROR;
    }

    // check for ; , just remvoe we dont need it
    size_t semiPos = hexStr.find(';');
    if (semiPos != std::string::npos) {
      hexStr = hexStr.substr(0, semiPos);
    }

    if (hexStr.find_first_not_of("0123456789abcdefABCDEF") !=
        std::string::npos) {
      return DECODE_ERROR;
    }

    errno = 0;
    char *endPtr = NULL;
    unsigned long chunkSize = std::strtoul(hexStr.c_str(), &endPtr, 16);
    if (endPtr == hexStr.c_str() || *endPtr != '\0' || errno == ERANGE)
      return DECODE_ERROR;

    // skip /r/n
    pos = crlfPos + 2;
    // we check for the end of the body
    if (chunkSize == 0) {
      // we didnt get the full last line
      if (pos + 2 > chunkedData.size())
        return DECODE_NEED_MORE;
      // we handle if there is optional trailer
      if (chunkedData[pos] != '\r' || chunkedData[pos + 1] != '\n') {
        // skip them all until we find empty line

        // for when we read all the trailer but we didnt read the final /r/n of the body
        bool foundEnd = false;
        while (pos < chunkedData.size()) {
          size_t trailEndPos = chunkedData.find("\r\n", pos);
          // incomplet trailer line
          if (trailEndPos == std::string::npos)
            return DECODE_NEED_MORE;
          // we found the end of the body
          if (trailEndPos == pos) {
            foundEnd = true;
            break;
          }
          pos = trailEndPos + 2;
        }
        if (!foundEnd)
          return DECODE_NEED_MORE;
      }
      completed = true;
      return DECODE_COMPLETE;
    }

    // we didnt read the full line
    if (pos + chunkSize + 2 > chunkedData.size()) {
      return DECODE_NEED_MORE;
    }
    decodedBody.append(chunkedData, pos, chunkSize);
    pos += chunkSize;

    // check for /r/n in the end of the line
    if (chunkedData[pos] != '\r' || chunkedData[pos + 1] != '\n') {
      return DECODE_ERROR;
    }
    pos += 2;
  }

  // need more data
  return DECODE_NEED_MORE;
}

bool ChunksDecoding::isComplete() const { return completed; }
