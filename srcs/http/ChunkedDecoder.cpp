#include "../../includes/http/ChunksDecoding.hpp"

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

    // check if number is in hex
    for (size_t i = 0; i < hexStr.size(); ++i) {
      char c = hexStr[i];
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F'))) {
        return DECODE_ERROR;
      }
    }
    // endptr returns a pointer to the first character after the number, we
    // check if it is the same as the start of the string, if it is then it
    // means that there was no valid number to convert
    char *endPtr = NULL;
    unsigned long chunkSize = std::strtoul(hexStr.c_str(), &endPtr, 16);
    if (endPtr == hexStr.c_str())
      return DECODE_ERROR; // conversion failed

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

void ChunksDecoding::reset() { completed = false; }
