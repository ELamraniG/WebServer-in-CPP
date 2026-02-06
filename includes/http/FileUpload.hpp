#pragma once

#include <string>
#include <vector>

#include "HTTPRequest.hpp"

struct UploadedFile {
  std::string filename;
  std::string contentType;
  std::string data;
};

class FileUpload {
public:
  FileUpload();

  bool parse(const HTTPRequest &request, std::vector<UploadedFile> &files);
  bool saveFile(const UploadedFile &file, const std::string &uploadDir);

private:
  std::string extractBoundary(const std::string &contentType) const;
};

