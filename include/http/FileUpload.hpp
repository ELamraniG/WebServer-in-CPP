#pragma once

#include <string>

#include "HTTPRequest.hpp"

struct FileData {
  std::string filename;
  std::string contentType;
  std::string data;
};

class FileUpload {
public:
  FileUpload();

  bool parseTheThing(const HTTPRequest &request, FileData &file);
  bool saveTheThing(const FileData &file, const std::string &uploadDir);

private:
  std::string extractBoundary(const std::string &contentType) const;
};
