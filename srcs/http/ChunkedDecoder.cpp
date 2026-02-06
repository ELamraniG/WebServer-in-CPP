#include "../../includes/http/ChunksDecoding.hpp"


bool ChunksDecoding::decode(const std::string &chunkedData,
                            std::string &decodedBody) {
  std::string::size_type pos = 0;

  while (pos < chunkedData.size()) {
   
    std::string::size_type crlfPos = chunkedData.find("\r\n", pos);
    if (crlfPos == std::string::npos) {
     
      return false;
    }

   
    std::string hexStr = chunkedData.substr(pos, crlfPos - pos);
    if (hexStr.empty()) {
      return false; 
    }

    
    
    std::string::size_type semiPos = hexStr.find(';');
    if (semiPos != std::string::npos) {
      hexStr = hexStr.substr(0, semiPos);
    }

    
    for (std::string::size_type i = 0; i < hexStr.size(); ++i) {
      char c = hexStr[i];
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F'))) {
        return false;
      }
    }

    char *endPtr = NULL;
    unsigned long chunkSize = std::strtoul(hexStr.c_str(), &endPtr, 16);
    if (endPtr == hexStr.c_str()) {
      return false;
    }

    
    pos = crlfPos + 2;

    
    if (chunkSize == 0) {
      
      
      if (pos + 2 > chunkedData.size()) {
        
        return false;
      }
      if (chunkedData[pos] != '\r' || chunkedData[pos + 1] != '\n') {
        
        
        while (pos < chunkedData.size()) {
          std::string::size_type trailerEnd = chunkedData.find("\r\n", pos);
          if (trailerEnd == std::string::npos) {
            return false;
          }
          if (trailerEnd == pos) {
            
            pos = trailerEnd + 2;
            break;
          }
          pos = trailerEnd + 2;
        }
      } else {
        pos += 2;
      }
      completed = true;
      return true;
    }

    
    if (pos + chunkSize > chunkedData.size()) {
      
      return false;
    }

    decodedBody.append(chunkedData, pos, chunkSize);
    pos += chunkSize;

    
    if (pos + 2 > chunkedData.size()) {
      return false; 
    }
    if (chunkedData[pos] != '\r' || chunkedData[pos + 1] != '\n') {
      return false; 
    }
    pos += 2;
  }

  
  return false;
}

bool ChunksDecoding::isComplete() const { return completed; }

void ChunksDecoding::reset() { completed = false; }
