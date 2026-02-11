
#include "../../includes/http/MethodHandler.hpp"
#include "../../includes/http/CGIHandler.hpp"
#include "../../includes/http/FileUpload.hpp"
#include "../../includes/http/RouteConfig.hpp"

#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

MethodHandler::MethodHandler() {}


static bool isAllowed(const std::string &method,
                            const RouteConfig &route) {
  const std::vector<std::string> &allowed = route.getAllowedMethods();
  // check if the method is allowed
  for (size_t i = 0; i < allowed.size(); i++) {
    if (allowed[i] == method)
      return true;
  }
  // if nothing is in the allowed then everything is allowed
  if (allowed.empty())
    return true;
  return false;
}


static std::string makeThePath(const std::string &root,
                               const std::string &uri) {
  std::string path = root;
  if (!path.empty() && path[path.size() - 1] == '/' && !uri.empty() &&
      uri[0] == '/') {
    path += uri.substr(1);
  } else {
    path += uri;
  }
  return path;
}


static bool readFileContent(const std::string &path, std::string &content) {
  std::ifstream file(path.c_str(), std::ios::binary);
  if (!file.is_open())
    return false;
  std::stringstream oss;
  oss << file.rdbuf();
  content = oss.str();
  file.close();
  return true;
}

static bool isDirectory(const std::string &path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0)
    return false;
  return S_ISDIR(st.st_mode);
}

static bool isValidFile(const std::string &path) {
  struct stat st;
  return (stat(path.c_str(), &st) == 0);
}

// html that shows all the files in the dir
static std::string getAllTheThings(const std::string &dirPath,
                                   const std::string &uri) {
  DIR *dir = opendir(dirPath.c_str());
  if (dir == NULL)
    return "";

  std::stringstream html;
  html << "<html>\n<body>\n";
  html << "<h1>Index of " << uri << "</h1>\n";

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    std::string name = entry->d_name;
    if (name == ".")
      continue;

    // figure out the link uri
    std::string linkUri = uri;
    if (!linkUri.empty() && linkUri[linkUri.size() - 1] != '/')
      linkUri += "/";

    // if its a subdirectory add / at end of name
    std::string fullEntryPath = dirPath;
    if (!fullEntryPath.empty() &&
        fullEntryPath[fullEntryPath.size() - 1] != '/')
      fullEntryPath += "/";
    fullEntryPath += name;

    if (isDirectory(fullEntryPath))
      name += "/";

    html << "<a href=\"" << linkUri << name << "\">" << name << "</a><br>\n";
  }
  closedir(dir);

  html << "</body>\n</html>\n";
  return html.str();
}

// quick way to make a simple error response
static Response putErr(int code, const std::string &msg) {
  Response resp;
  resp.statusCode = code;
  resp.contentType = "text/html";

  std::stringstream body;
  body << "<html><body><h1>" << code << " " << msg << "</h1></body></html>";
  resp.body = body.str();
  return resp;
}

std::string MethodHandler::getTheFileType(const std::string &path) const {
  size_t dotPos = path.find_last_of('.');
  if (dotPos == std::string::npos)
    return "application/octet-stream";

  std::string ext = path.substr(dotPos);
  for (size_t i = 0; i < ext.size(); i++)
    ext[i] = std::tolower(ext[i]);
  if (ext == ".html" || ext == ".htm")
    return "text/html";
  if (ext == ".css")
    return "text/css";
  if (ext == ".js")
    return "application/javascript";
  if (ext == ".jpg" || ext == ".jpeg")
    return "image/jpeg";
  if (ext == ".ico")
    return "image/x-icon";
  if (ext == ".txt")
    return "text/plain";

  // default
  return "application/octet-stream";
}

Response MethodHandler::handleGET(const HTTPRequest &request,
                                  const RouteConfig &route) {
  // is it allowed
  if (!isAllowed("GET", route))
    return putErr(405, "Method is not allowed");

  // make the path
  std::string ourFilePath = makeThePath(route.getRoot(), request.getURI());

  // handledir
  if (isDirectory(ourFilePath)) {
    // get the idx
    std::string indexFile = route.getIndex();
    // check if we have index file in theconfig
    if (indexFile.empty() == false) {
      std::string indexPath = ourFilePath;

      // add / if the path doesnt end with it
      if (!indexPath.empty() && indexPath[indexPath.size() - 1] != '/')
        indexPath += "/";

      indexPath += indexFile;

      if (isValidFile(indexPath)) {
        std::string content;
        if (!readFileContent(indexPath, content))
          return putErr(500, "Internal Server Error");

        Response resp;
        resp.statusCode = 200;
        resp.contentType = getTheFileType(indexPath);
        resp.body = content;
        return resp;
      }
    }

    // no index in the config, try autoindex
    if (route.getAutoindex()) {
      std::string tmpp = getAllTheThings(ourFilePath, request.getURI());
      if (tmpp.empty())
        return putErr(500, "Internal Server Error");

      Response resp;
      resp.statusCode = 200;
      resp.contentType = "text/html";
      resp.body = tmpp;
      return resp;
    }

    // directory, no index, no autoindex -> forbidden
    return putErr(403, "Forbidden");
  }

  //  if file doesnt exist 404
  if (!isValidFile(ourFilePath))
    return putErr(404, "Not Found");

  std::string content;
  // can read file 500
  if (!readFileContent(ourFilePath, content))
    return putErr(500, "Internal Server Error");

  Response resp;
  resp.statusCode = 200;
  resp.contentType = getTheFileType(ourFilePath);
  resp.body = content;
  return resp;
}

Response MethodHandler::handlePOST(const HTTPRequest &request,
                                   const RouteConfig &route) {

  if (!isAllowed("POST", route))
    return putErr(405, "Method Not Allowed");


  if (route.getMaxBodySize() > 0 &&
      request.getBody().size() > route.getMaxBodySize()) {
    return putErr(413, "Payload Too Large");
  }

  CGIHandler cgiHandler;
  if (cgiHandler.isCGIRequest(request.getURI(), route)) {
    CGIResult cgiResult = cgiHandler.execute(request, route);

    Response resp;
    if (!cgiResult.success) {

      resp.statusCode = cgiResult.statusCode;
      resp.contentType = "text/html";
      std::stringstream errBody;
      errBody << "<html><body><h1>" << cgiResult.statusCode
              << " CGI Error</h1><p>" << cgiResult.errorMessage
              << "</p></body></html>";
      resp.body = errBody.str();
      return resp;
    }

    resp.statusCode = cgiResult.statusCode;
    resp.body = cgiResult.body;
    resp.headers = cgiResult.headers;

    std::map<std::string, std::string>::const_iterator it;
    it = cgiResult.headers.find("content-type");
    if (it != cgiResult.headers.end())
      resp.contentType = it->second;
    else
      resp.contentType = "text/html";

    return resp;
  }

  std::string contentType = request.getHeader("content-type");
  if (contentType.find("multipart/form-data") != std::string::npos) {

    FileUpload uploader;
    std::vector<UploadedFile> files;

    if (!uploader.parse(request, files)) {
      return putErr(400, "Bad Request");
    }

    if (files.empty()) {
      return putErr(400, "Bad Request");
    }

    std::string uploadDir = route.getUploadStore();
    if (uploadDir.empty()) {
      std::cerr << "MethodHandler: no upload directory configured" << std::endl;
      return putErr(500, "Internal Server Error");
    }

    bool allOk = true;
    std::stringstream resultBody;
    resultBody << "<html><body>\n";
    resultBody << "<h1>Upload Complete</h1>\n<ul>\n";

    for (size_t i = 0; i < files.size(); i++) {
      if (uploader.saveFile(files[i], uploadDir)) {
        resultBody << "<li>" << files[i].filename << " - uploaded ok</li>\n";
      } else {
        resultBody << "<li>" << files[i].filename << " - FAILED</li>\n";
        allOk = false;
      }
    }

    resultBody << "</ul>\n</body></html>";

    Response resp;
    resp.statusCode = allOk ? 201 : 500;
    resp.contentType = "text/html";
    resp.body = resultBody.str();
    return resp;
  }

  Response resp;
  resp.statusCode = 200;
  resp.contentType = "text/html";
  resp.body = "<html><body><h1>200 OK</h1>"
              "<p>Data received successfully.</p></body></html>";
  return resp;
}

Response MethodHandler::handleDELETE(const HTTPRequest &request,
                                     const RouteConfig &route) {
  // check if delete is allowed
  if (!isAllowed("DELETE", route))
    return putErr(405, "Method Not Allowed");

  std::string ourFilePath = makeThePath(route.getRoot(), request.getURI());

  // we cant dellete dirs
  if (isDirectory(ourFilePath))
    return putErr(403, "Forbidden");

  if (!isValidFile(ourFilePath))
    return putErr(404, "Not Found");

  if (std::remove(ourFilePath.c_str()) != 0) {
    std::cerr << "MethodHandler: failed to delete file: " << ourFilePath
              << std::endl;
    return putErr(500, "Internal Server Error");
  }

  Response resp;
  resp.statusCode = 200;
  resp.contentType = "text/html";
  resp.body = "<html><body><h1>200 OK</h1>"
              "<p>File deleted successfully.</p></body></html>";
  return resp;
}
