#include "../../includes/http/FileUpload.hpp"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

FileUpload::FileUpload() {}

// ---------------------------------------------------------------------------
// Extract the boundary string from the Content-Type header value.
// Expected format:  multipart/form-data; boundary=----WebKitFormBoundary...
// ---------------------------------------------------------------------------
std::string FileUpload::extractBoundary(const std::string &contentType) const {
  std::string::size_type pos = contentType.find("boundary=");
  if (pos == std::string::npos)
    return "";
  std::string boundary = contentType.substr(pos + 9);

  // Trim "" nd spaces 
  while (!boundary.empty() && (boundary[0] == ' ' || boundary[0] == '"'))
    boundary.erase(0, 1);
  while (!boundary.empty() && (boundary[boundary.size() - 1] == ' ' ||
                               boundary[boundary.size() - 1] == '"'))
    boundary.erase(boundary.size() - 1, 1);

  return boundary;
}
static std::string strTrim(const std::string &s) {
  std::string::size_type start = 0;
  while (start < s.size() &&
         (s[start] == ' ' || s[start] == '\t' || s[start] == '\r'))
    ++start;
  std::string::size_type end = s.size();
  while (end > start &&
         (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r'))
    --end;
  return s.substr(start, end - start);
}

// ---------------------------------------------------------------------------
// Extract a named attribute value from a header value string.
// e.g. extractAttribute("form-data; name=\"file\"; filename=\"pic.jpg\"",
//                        "filename")  -->  "pic.jpg"
// ---------------------------------------------------------------------------
static std::string extractAttribute(const std::string &header,
                                    const std::string &attr) {
  std::string key = attr + "=\"";
  std::string::size_type pos = header.find(key);
  if (pos == std::string::npos) {
    // Try without quotes:  attr=value
    key = attr + "=";
    pos = header.find(key);
    if (pos == std::string::npos)
      return "";
    std::string::size_type valStart = pos + key.size();
    std::string::size_type valEnd = header.find(';', valStart);
    if (valEnd == std::string::npos)
      valEnd = header.size();
    return strTrim(header.substr(valStart, valEnd - valStart));
  }
  std::string::size_type valStart = pos + key.size();
  std::string::size_type valEnd = header.find('"', valStart);
  if (valEnd == std::string::npos)
    return "";
  return header.substr(valStart, valEnd - valStart);
}

// ---------------------------------------------------------------------------
// Parse the headers section of a single MIME part.
// Returns a map of lowercase-header-name -> value.
// ---------------------------------------------------------------------------
static std::map<std::string, std::string>
parsePartHeaders(const std::string &headerBlock) {
  std::map<std::string, std::string> hdrs;
  std::string::size_type pos = 0;

  while (pos < headerBlock.size()) {
    std::string::size_type lineEnd = headerBlock.find('\n', pos);
    if (lineEnd == std::string::npos)
      lineEnd = headerBlock.size();

    std::string line = strTrim(headerBlock.substr(pos, lineEnd - pos));
    pos = lineEnd + 1;

    if (line.empty())
      continue;

    std::string::size_type colon = line.find(':');
    if (colon == std::string::npos)
      continue;

    std::string name = strTrim(line.substr(0, colon));
    std::string value = strTrim(line.substr(colon + 1));

    // Lowercase the header name for case-insensitive lookup
    for (std::string::size_type i = 0; i < name.size(); ++i)
      name[i] =
          static_cast<char>(std::tolower(static_cast<unsigned char>(name[i])));

    hdrs[name] = value;
  }
  return hdrs;
}

// ---------------------------------------------------------------------------
// parse()  –  Break the multipart/form-data body into individual files.
//
// Multipart format (simplified):
//   --<boundary>\r\n
//   Content-Disposition: form-data; name="file"; filename="photo.jpg"\r\n
//   Content-Type: image/jpeg\r\n
//   \r\n
//   <binary data>\r\n
//   --<boundary>--\r\n          ← closing boundary
// ---------------------------------------------------------------------------
bool FileUpload::parseTheThing(const HTTPRequest &request,
                       std::vector<FileData> &files) {
  std::string contentType = request.getHeader("content-type");
  std::string boundary = extractBoundary(contentType);
  if (boundary.empty()) {
    std::cerr << "no extract boundary" << std::endl;
    return false;
  }
  // -- is added to the boundary par deafault
  const std::string &body = request.getBody();
  std::string delimiter = "--" + boundary;
  // Find the first boundary
  std::string::size_type pos = body.find(delimiter);
  if (pos == std::string::npos) {
    std::cerr << "no boundary" << std::endl;
    return false;
  }

  // Move past the first boundary line
  pos += delimiter.size();
  if (pos < body.size() && body[pos] == '\r')
    ++pos;
  if (pos < body.size() && body[pos] == '\n')
    ++pos;

  while (pos < body.size()) {
    // Find the next boundary (marks the end of this part)
    std::string::size_type nextBound = body.find(delimiter, pos);
    if (nextBound == std::string::npos)
      break;

    // The part data is between pos and nextBound
    // Subtract trailing \r\n that precedes the boundary marker
    std::string::size_type partEnd = nextBound;
    if (partEnd >= 2 && body[partEnd - 2] == '\r' && body[partEnd - 1] == '\n')
      partEnd -= 2;
    else if (partEnd >= 1 && body[partEnd - 1] == '\n')
      --partEnd;

    std::string part = body.substr(pos, partEnd - pos);

    // Split part into headers and data at the blank line (\r\n\r\n)
    std::string::size_type headerEnd = part.find("\r\n\r\n");
    std::string::size_type sepLen = 4;
    if (headerEnd == std::string::npos) {
      headerEnd = part.find("\n\n");
      sepLen = 2;
    }

    if (headerEnd != std::string::npos) {
      std::string headerBlock = part.substr(0, headerEnd);
      std::string partData = part.substr(headerEnd + sepLen);

      std::map<std::string, std::string> hdrs = parsePartHeaders(headerBlock);

      // We only care about parts that carry a file (have a filename)
      std::string disposition;
      std::map<std::string, std::string>::const_iterator it =
          hdrs.find("content-disposition");
      if (it != hdrs.end())
        disposition = it->second;

      std::string filename = extractAttribute(disposition, "filename");
      if (!filename.empty()) {
        FileData file;
        file.filename = filename;

        std::map<std::string, std::string>::const_iterator ctIt =
            hdrs.find("content-type");
        if (ctIt != hdrs.end())
          file.contentType = ctIt->second;
        else
          file.contentType = "application/octet-stream";

        file.data = partData;
        files.push_back(file);
      }
    }

    // Advance past this boundary
    pos = nextBound + delimiter.size();

    // Check for closing delimiter ("--")
    if (pos + 1 < body.size() && body[pos] == '-' && body[pos + 1] == '-')
      break;

    // Skip CRLF after boundary
    if (pos < body.size() && body[pos] == '\r')
      ++pos;
    if (pos < body.size() && body[pos] == '\n')
      ++pos;
  }

  return !files.empty();
}

bool FileUpload::saveTheThing(const FileData &file,
                          const std::string &uploadDir) {
  if (file.filename.empty()) {
    std::cerr << "error in the filename" << std::endl;
    return false;
  }

  // Ensure upload directory exists
  struct stat st;
  if (stat(uploadDir.c_str(), &st) != 0) {
    std::cerr << " error in the directory "
              << uploadDir << std::endl;
    return false;
  }
  if (!S_ISDIR(st.st_mode)) {
    std::cerr << "FileUpload: not a directory: " << uploadDir << std::endl;
    return false;
  }

  // Sanitise the filename – strip path components and dangerous characters
  std::string safeName = file.filename;
  std::string::size_type slash = safeName.find_last_of("/\\");
  if (slash != std::string::npos)
    safeName = safeName.substr(slash + 1);

  // Reject null bytes and control characters
  for (std::string::size_type i = 0; i < safeName.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(safeName[i]);
    if (c < 0x20 || c == 0x7F) {
      std::cerr << "FileUpload: unsafe character in filename" << std::endl;
      return false;
    }
  }
  if (safeName.empty() || safeName == "." || safeName == "..")
    safeName = "upload";

  // Build full path
  std::string path = uploadDir;
  if (!path.empty() && path[path.size() - 1] != '/')
    path += '/';
  path += safeName;

  // If the file already exists, generate a unique name
  if (stat(path.c_str(), &st) == 0) {
    // Find extension
    std::string base = safeName;
    std::string ext;
    std::string::size_type dot = safeName.find_last_of('.');
    if (dot != std::string::npos) {
      base = safeName.substr(0, dot);
      ext = safeName.substr(dot); // includes the dot
    }

    bool found = false;
    for (int i = 1; i < 10000; ++i) {
      std::ostringstream oss;
      oss << "_" << i;
      std::string candidate =
          uploadDir + (uploadDir[uploadDir.size() - 1] != '/' ? "/" : "") +
          base + oss.str() + ext;
      if (stat(candidate.c_str(), &st) != 0) {
        path = candidate;
        found = true;
        break;
      }
    }
    if (!found) {
      std::cerr << "FileUpload: could not find unique name for: "
                << safeName << std::endl;
      return false;
    }
  }

  // Write in binary mode to preserve exact data
  std::ofstream ofs(path.c_str(),
                    std::ios::out | std::ios::binary | std::ios::trunc);
  if (!ofs.is_open()) {
    std::cerr << "FileUpload::saveFile: could not open file for writing: "
              << path << " (" << std::strerror(errno) << ")" << std::endl;
    return false;
  }

  ofs.write(file.data.c_str(), static_cast<std::streamsize>(file.data.size()));
  if (!ofs.good()) {
    std::cerr << "FileUpload::saveFile: write error for: " << path << std::endl;
    ofs.close();
    return false;
  }

  ofs.close();
  std::cout << "FileUpload: saved " << file.filename << " (" << file.data.size()
            << " bytes) -> " << path << std::endl;
  return true;
}
