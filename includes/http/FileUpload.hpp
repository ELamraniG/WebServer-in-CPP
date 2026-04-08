#pragma once

#include <string>
#include <vector>

#include "HTTPRequest.hpp"

struct FileData {
  std::string filename;
  std::string contentType;
  std::string data;
};

class FileUpload {
public:
  FileUpload();

  bool parseTheThing(const HTTPRequest &request,
                     std::vector<FileData> &files);
  bool saveTheThing(const FileData &file, const std::string &uploadDir);

private:
  std::string extractBoundary(const std::string &contentType) const;
};
