#include "../../includes/http/MethodHandler.hpp"
#include "../../includes/http/CGIHandler.hpp"
#include "../../includes/http/FileUpload.hpp"
#include "../../includes/http/RouteConfig.hpp"

#include <algorithm>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

MethodHandler::MethodHandler() {}

// ─────────────────────────────────────────────
//  HELPERS
// ─────────────────────────────────────────────

static bool isAllowed(const std::string &method, const RouteConfig &route) {
  const std::vector<std::string> &allowed = route.getAllowedMethods();
  if (allowed.empty())
    return true;
  for (size_t i = 0; i < allowed.size(); i++)
    if (allowed[i] == method)
      return true;
  return false;
}


static std::string RemoveQueryString(const std::string &uri) {
  size_t pos = uri.find('?');
  if (pos != std::string::npos) {
    return uri.substr(0, pos);
  } else {
    return uri;
  }
}

static bool isItUnsafe(const std::string &uri) {
  std::string tmpcheck = RemoveQueryString(uri);
  if (tmpcheck.find('\\') != std::string::npos)
    return true;

  size_t start = 0;
  while (start <= tmpcheck.size()) {
    size_t slash = tmpcheck.find('/', start); 
    std::string issafe;
    if (slash == std::string::npos) {
      issafe = tmpcheck.substr(start);
    } else {
      issafe = tmpcheck.substr(start, slash - start);
    }
    if (issafe == "..")
      return true;
    if (slash == std::string::npos)
      break;
    start = slash + 1;
  }
  return false;
}

static std::string makeThePath(const std::string &root,
                               const std::string &uri) {
  std::string clean = RemoveQueryString(uri);
  std::string path = root;
  if (!path.empty() && path[path.size() - 1] == '/' && !clean.empty() &&
      clean[0] == '/')
    path += clean.substr(1);
  else
    path += clean;
  return path;
}

static bool readFileContent(const std::string &path, std::string &content) {
  std::ifstream file(path.c_str(), std::ios::binary);
  if (!file.is_open())
    return false;
  std::ostringstream oss;
  oss << file.rdbuf();
  content = oss.str();
  return true;
}

static bool isDirectory(const std::string &path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0)
    return false;
  return S_ISDIR(st.st_mode);
}

static bool fileExists(const std::string &path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}

static bool isReadable(const std::string &path) {
  return access(path.c_str(), R_OK) == 0;
}

static bool canDeletePath(const std::string &path) {
  size_t slash = path.find_last_of('/');
  std::string parent;

  if (slash == std::string::npos)
    parent = ".";
  else if (slash == 0)
    parent = "/";
  else
    parent = path.substr(0, slash);

  return access(parent.c_str(), W_OK | X_OK) == 0;
}

// Build a minimal error response (used when no custom error page exists)
static Response makeError(int code, const std::string &msg) {
  Response resp;
  resp.statusCode = code;
  resp.contentType = "text/html";

  std::ostringstream body;
  body << "<!DOCTYPE html>\n<html><head><title>" << code << " " << msg
       << "</title></head>\n"
       << "<body><h1>" << code << " " << msg << "</h1></body></html>\n";
  resp.body = body.str();
  return resp;
}

// Common preamble: method-allowed + path-traversal check.
// Returns true if the request should be rejected (resp is filled).
static bool commonChecks(const std::string &method, const HTTPRequest &request,
                         const RouteConfig &route, Response &resp) {
  if (!isAllowed(method, route)) {
    resp = makeError(405, "Method Not Allowed");
    return true;
  }
  if (isItUnsafe(request.getURI())) {
    resp = makeError(403, "Forbidden");
    return true;
  }
  return false;
}

// Build a 301 redirect response
static Response makeRedirect(const std::string &location) {
  Response resp;
  resp.statusCode = 301;
  resp.contentType = "text/html";
  resp.headers["Location"] = location;
  resp.body = "<html><body>Moved Permanently</body></html>";
  return resp;
}

// Dispatch to CGI and build the Response.
// Returns true if the URI was a CGI request (resp is filled).
static bool tryCGI(const HTTPRequest &request, const RouteConfig &route,
                   Response &resp) {
  CGIHandler cgiHandler;
  if (!cgiHandler.isCGIRequest(request.getURI(), route))
    return false;

  CGIResult cgiResult = cgiHandler.execute(request, route);

  if (!cgiResult.success) {
    std::ostringstream errBody;
    errBody << "<!DOCTYPE html>\n<html><body><h1>" << cgiResult.statusCode
            << " CGI Error</h1><p>" << cgiResult.errorMessage
            << "</p></body></html>\n";
    resp.statusCode = cgiResult.statusCode;
    resp.contentType = "text/html";
    resp.body = errBody.str();
    return true;
  }

  resp.statusCode = cgiResult.statusCode;
  resp.body = cgiResult.body;
  resp.headers = cgiResult.headers;

  std::map<std::string, std::string>::const_iterator it =
      cgiResult.headers.find("content-type");
  if (it != cgiResult.headers.end()) {
    resp.contentType = it->second;
  } else {
    resp.contentType = "text/html";
  }
  return true;
}

// Autoindex – sorted, shows trailing slash for subdirs
static std::string buildAutoindex(const std::string &dirPath,
                                  const std::string &uri) {
  DIR *dir = opendir(dirPath.c_str());
  if (!dir)
    return "";

  // collect entries
  std::vector<std::string> entries;
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    std::string name = entry->d_name;
    if (name == "." || name == "..")
      continue;
    entries.push_back(name);
  }
  closedir(dir);
  std::sort(entries.begin(), entries.end());

  // base URI must end with /
  std::string base = uri;
  if (base.empty() || base[base.size() - 1] != '/')
    base += "/";

  std::ostringstream html;
  html << "<!DOCTYPE html>\n<html>\n<head>\n"
       << "<title>Index of " << uri << "</title>\n"
       << "<style>body{font-family:monospace;padding:1em}"
       << "a{display:block;padding:2px 0}</style>\n"
       << "</head>\n<body>\n"
       << "<h1>Index of " << uri << "</h1><hr>\n";

  for (size_t i = 0; i < entries.size(); i++) {
    const std::string &name = entries[i];
    std::string fullPath = dirPath;
    if (fullPath[fullPath.size() - 1] != '/')
      fullPath += "/";
    fullPath += name;

    std::string display = name;
    if (isDirectory(fullPath))
      display += "/";

    html << "<a href=\"" << base << display << "\">" << display << "</a>\n";
  }

  html << "<hr></body>\n</html>\n";
  return html.str();
}

// ─────────────────────────────────────────────
//  MIME TYPES  (extended for 42 evaluation)
// ─────────────────────────────────────────────

std::string MethodHandler::getTheFileType(const std::string &path) const {
  size_t slashPos = path.find_last_of('/');
  size_t dotPos = path.find_last_of('.');
  // Dot must exist and appear after the last slash to be a real extension
  if (dotPos == std::string::npos ||
      (slashPos != std::string::npos && dotPos < slashPos))
    return "application/octet-stream";

  std::string ext = path.substr(dotPos);
  for (size_t i = 0; i < ext.size(); i++)
    ext[i] =
        static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i])));

  // text
  if (ext == ".html" || ext == ".htm")
    return "text/html";
  if (ext == ".css")
    return "text/css";
  if (ext == ".txt")
    return "text/plain";
  if (ext == ".csv")
    return "text/csv";
  if (ext == ".xml")
    return "text/xml";

  // scripts / data
  if (ext == ".js")
    return "application/javascript";
  if (ext == ".json")
    return "application/json";
  if (ext == ".pdf")
    return "application/pdf";
  if (ext == ".zip")
    return "application/zip";

  // images
  if (ext == ".jpg" || ext == ".jpeg")
    return "image/jpeg";
  if (ext == ".png")
    return "image/png";
  if (ext == ".gif")
    return "image/gif";
  if (ext == ".bmp")
    return "image/bmp";
  if (ext == ".svg")
    return "image/svg+xml";
  if (ext == ".ico")
    return "image/x-icon";
  if (ext == ".webp")
    return "image/webp";

  // audio / video
  if (ext == ".mp3")
    return "audio/mpeg";
  if (ext == ".mp4")
    return "video/mp4";
  if (ext == ".webm")
    return "video/webm";

  // fonts
  if (ext == ".woff")
    return "font/woff";
  if (ext == ".woff2")
    return "font/woff2";
  if (ext == ".ttf")
    return "font/ttf";

  return "application/octet-stream";
}

// ─────────────────────────────────────────────
//  GET
// ─────────────────────────────────────────────

Response MethodHandler::handleGET(const HTTPRequest &request,
                                  const RouteConfig &route) {
  Response resp;
  if (commonChecks("GET", request, route, resp))
    return resp;

  // 1. Handle HTTP redirect configured on the route
  if (!route.getRedirect().empty())
    return makeRedirect(route.getRedirect());

  // 2. CGI dispatch
  if (tryCGI(request, route, resp))
    return resp;

  std::string ourFilePath = makeThePath(route.getRoot(), request.getURI());

  // 3. Directory handling
  if (isDirectory(ourFilePath)) {
    // 3a. Redirect bare directory path without trailing slash
    std::string currentUri = RemoveQueryString(request.getURI());
    if (!currentUri.empty() && currentUri[currentUri.size() - 1] != '/')
      return makeRedirect(currentUri + "/");

    // 3b. Try configured index file
    std::string indexFile = route.getIndex();
    if (!indexFile.empty()) {
      std::string indexPath = ourFilePath;
      if (indexPath[indexPath.size() - 1] != '/')
        indexPath += "/";
      indexPath += indexFile;

      if (fileExists(indexPath)) {
        // Construct a new URI including the index file for CGI check
        std::string baseURI = request.getURI();
        size_t queryPos = baseURI.find('?');
        std::string justPath;
        std::string queryPart;
        if (queryPos != std::string::npos) {
          justPath = baseURI.substr(0, queryPos);
          queryPart = baseURI.substr(queryPos);
        } else {
          justPath = baseURI;
          queryPart = "";
        }
        
        if (!justPath.empty() && justPath[justPath.size() - 1] != '/')
            justPath += "/";
            
        std::string indexedURI = justPath + indexFile + queryPart;
        
        HTTPRequest indexReq = request;
        indexReq.setURI(indexedURI);
        
        if (tryCGI(indexReq, route, resp))
          return resp;

        if (!isReadable(indexPath))
          return makeError(403, "Forbidden");

        std::string content;
        if (!readFileContent(indexPath, content))
          return makeError(500, "Internal Server Error");

        resp.statusCode = 200;
        resp.contentType = getTheFileType(indexPath);
        resp.body = content;
        return resp;
      }
    }

    // 3c. Autoindex
    if (route.getAutoindex()) {
      std::string listing =
          buildAutoindex(ourFilePath, RemoveQueryString(request.getURI()));
      if (listing.empty())
        return makeError(500, "Internal Server Error");

      resp.statusCode = 200;
      resp.contentType = "text/html";
      resp.body = listing;
      return resp;
    }

    // 3d. Directory exists but no index and no autoindex
    return makeError(403, "Forbidden");
  }

  // 4. File not found
  if (!fileExists(ourFilePath))
    return makeError(404, "Not Found");

  // 5. File exists but not readable (permissions)
  if (!isReadable(ourFilePath))
    return makeError(403, "Forbidden");

  // 6. Serve the file
  std::string content;
  if (!readFileContent(ourFilePath, content))
    return makeError(500, "Internal Server Error");

  resp.statusCode = 200;
  resp.contentType = getTheFileType(ourFilePath);
  resp.body = content;
  return resp;
}

// ─────────────────────────────────────────────
//  POST
// ─────────────────────────────────────────────

Response MethodHandler::handlePOST(const HTTPRequest &request,
                                   const RouteConfig &route) {
  Response resp;
  if (commonChecks("POST", request, route, resp))
    return resp;

  // 1. Handle HTTP redirect configured on the route
  if (!route.getRedirect().empty())
    return makeRedirect(route.getRedirect());

  // 2. Body size limit (413 Payload Too Large)
  if (route.getMaxBodySize() > 0 &&
      request.getBody().size() > route.getMaxBodySize())
    return makeError(413, "Payload Too Large");

  // 3. CGI dispatch
  if (tryCGI(request, route, resp))
    return resp;

  // 4. Multipart file upload
  std::string contentType = request.getHeader("content-type");
  if (contentType.find("multipart/form-data") != std::string::npos) {
    FileUpload uploader;
    std::vector<FileData> files;

    if (!uploader.parseTheThing(request, files) || files.empty())
      return makeError(400, "Bad Request");

    std::string uploadDir = route.getUploadStore();
    if (uploadDir.empty()) {
      std::cerr << "MethodHandler: no upload directory configured\n";
      return makeError(500, "Internal Server Error");
    }

    // Verify upload directory exists
    if (!fileExists(uploadDir) || !isDirectory(uploadDir)) {
      std::cerr << "MethodHandler: upload directory missing: " << uploadDir
                << "\n";
      return makeError(500, "Internal Server Error");
    }

    bool allSaved = true;
    std::ostringstream res;
    res << "<!DOCTYPE html>\n<html><body>\n"
        << "<h1>Upload Complete</h1>\n<ul>\n";

    for (size_t i = 0; i < files.size(); i++) {
      if (uploader.saveTheThing(files[i], uploadDir))
        res << "<li>" << files[i].filename << " – uploaded ok</li>\n";
      else {
        res << "<li>" << files[i].filename << " – FAILED</li>\n";
        allSaved = false;
      }
    }
    res << "</ul>\n</body></html>\n";

    if (allSaved) {
      resp.statusCode = 201;
    } else {
      resp.statusCode = 500;
    }
    resp.contentType = "text/html";
    if (allSaved) {
      std::string location = RemoveQueryString(request.getURI());
      if (location.empty())
        location = "/";
      else if (location[0] != '/')
        location = "/" + location;
      resp.headers["Location"] = location;
    }
    resp.body = res.str();
    return resp;
  }

  // 5. application/x-www-form-urlencoded or raw body
  //    42 subject does not require processing it, but we must not silently
  //    discard it. Return 200 with a proper acknowledgement.
  resp.statusCode = 200;
  resp.contentType = "text/html";
  resp.body = "<!DOCTYPE html>\n<html><body>"
              "<h1>200 OK</h1>"
              "<p>Request body received.</p>"
              "</body></html>\n";
  return resp;
}

// ─────────────────────────────────────────────
//  DELETE
// ─────────────────────────────────────────────

Response MethodHandler::handleDELETE(const HTTPRequest &request,
                                     const RouteConfig &route) {
  Response resp;
  if (commonChecks("DELETE", request, route, resp))
    return resp;

  std::string ourFilePath = makeThePath(route.getRoot(), request.getURI());

  // Cannot delete directories
  if (isDirectory(ourFilePath))
    return makeError(403, "Forbidden");

  // 404 before attempting removal
  if (!fileExists(ourFilePath))
    return makeError(404, "Not Found");

  if (!canDeletePath(ourFilePath))
    return makeError(403, "Forbidden");

  if (std::remove(ourFilePath.c_str()) != 0) {
    std::cerr << "MethodHandler: failed to delete: " << ourFilePath << "\n";
    return makeError(500, "Internal Server Error");
  }

  // 204 No Content – no body, no content-type
  resp.statusCode = 204;
  resp.body = "";
  return resp;
}