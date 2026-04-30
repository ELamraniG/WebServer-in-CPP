#include "../../include/http/MethodHandler.hpp"
#include "../../include/http/FileUpload.hpp"
#include "../../include/http/RouteConfig.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

MethodHandler::MethodHandler() {}

static std::map<std::string, std::string> AllSession;
static unsigned long gSessionCounter = 0;

std::string extractExtention(const std::string &uri) {
  size_t dot;
  size_t queryStartAt;
  std::string extension;

  dot = uri.find_last_of('.');
  if (dot == std::string::npos)
    return ("");
  extension = uri.substr(dot);
  queryStartAt = extension.find('?');
  if (queryStartAt != std::string::npos)
    extension = extension.substr(0, queryStartAt);
  return (extension);
}

static std::string makeSessionId() {
  std::ostringstream SesId;

  SesId << "sessionstuffetcc";
  SesId << static_cast<unsigned long>(std::time(NULL));
  SesId << "_";
  SesId << gSessionCounter++;
  return SesId.str();
}

static void ensureSession(const HTTPRequest &request, Response &resp) {
  std::string sessionId = request.getCookie("SESSION_ID");
  if (!sessionId.empty()) {
    if (AllSession.find(sessionId) != AllSession.end())
      return;
  }
  std::string newSessionId = makeSessionId();
  while (AllSession.find(newSessionId) != AllSession.end())
    newSessionId = makeSessionId();
  AllSession[newSessionId] = "";
  if (resp.headers.find("Set-Cookie") == resp.headers.end()) {
    resp.headers["Set-Cookie"] =
        "SESSION_ID=" + newSessionId +
        "; Path=/\r\n"
        "Set-Cookie: member1=ELAMRANI; Path=/; Max-Age=31536000\r\n"
        "Set-Cookie: member2=REDA; Path=/; Max-Age=31536000\r\n"
        "Set-Cookie: member3=SIMO; Path=/; Max-Age=31536000";
  }
}

static Response applySession(const HTTPRequest &request, Response resp) {
  ensureSession(request, resp);
  return resp;
}

static bool isAllowed(const std::string &method, const RouteConfig &route) {
  const std::vector<std::string> &allowed = route.getAllowedMethods();
  if (allowed.empty())
    return (true);
  for (size_t i = 0; i < allowed.size(); i++)
    if (allowed[i] == method)
      return (true);
  return (false);
}

static std::string RemoveQueryString(const std::string &uri) {
  size_t pos;

  pos = uri.find('?');
  if (pos != std::string::npos) {
    return (uri.substr(0, pos));
  } else {
    return (uri);
  }
}

static bool isItUnsafe(const std::string &uri) {
  size_t start;
  size_t slash;

  std::string tmpcheck = RemoveQueryString(uri);
  if (tmpcheck.find('\\') != std::string::npos)
    return (true);
  start = 0;
  while (start <= tmpcheck.size()) {
    slash = tmpcheck.find('/', start);
    std::string issafe;
    if (slash == std::string::npos) {
      issafe = tmpcheck.substr(start);
    } else {
      issafe = tmpcheck.substr(start, slash - start);
    }
    if (issafe == "..")
      return (true);
    if (slash == std::string::npos)
      break;
    start = slash + 1;
  }
  return (false);
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
  return (path);
}

static bool readFileContent(const std::string &path, std::string &content) {
  std::ifstream file(path.c_str(), std::ios::binary);
  if (!file.is_open())
    return (false);
  std::ostringstream oss;
  oss << file.rdbuf();
  content = oss.str();
  return (true);
}

static bool isDirectory(const std::string &path) {
  struct stat st;

  if (stat(path.c_str(), &st) != 0)
    return (false);
  return (S_ISDIR(st.st_mode));
}

static bool fileExists(const std::string &path) {
  struct stat st;

  return (stat(path.c_str(), &st) == 0);
}

static bool isReadable(const std::string &path) {
  return (access(path.c_str(), R_OK) == 0);
}

static bool canDeletePath(const std::string &path) {
  size_t slash;

  slash = path.find_last_of('/');
  std::string parent;
  if (slash == std::string::npos)
    parent = ".";
  else if (slash == 0)
    parent = "/";
  else
    parent = path.substr(0, slash);
  return (access(parent.c_str(), W_OK | X_OK) == 0);
}

Response MethodHandler::makeError(int code, const std::string &msg,
                                  const RouteConfig &route) {
  Response resp;

  resp.statusCode = code;
  resp.contentType = "text/html";

  const std::map<int, std::string> &errPages = route.getErrorPages();
  std::map<int, std::string>::const_iterator it = errPages.find(code);
  if (it != errPages.end()) {
	  std::string path = it->second;
    std::string content;
    if (readFileContent(path, content)) {
		resp.body = content;
		return resp;
    }
    std::string rootPath = makeThePath(route.getRoot(), path);
	// std::cout << "DEBUG .. " << rootPath << std::endl; // FIXME: cannot find 504 cgi timeout page
    if (readFileContent(rootPath, content)) {
      resp.body = content;
      return resp;
    }
  }

  std::ostringstream body;
  body << "<!DOCTYPE html>\n<html><head><title>" << code << " " << msg
       << "</title></head>\n"
       << "<body><h1>" << code << " " << msg << "</h1></body></html>\n";
  resp.body = body.str();
  return resp;
}

static bool isSafeAndAllowed(const std::string &method,
                             const HTTPRequest &request,
                             const RouteConfig &route, Response &resp) {
  if (!isAllowed(method, route)) {
    resp = MethodHandler::makeError(405, "Method Not Allowed", route);
    return true;
  }
  if (isItUnsafe(request.getURI())) {
    resp = MethodHandler::makeError(403, "Forbidden", route);
    return true;
  }
  return false;
}

// 301 redirect
static Response makeRedirect(const std::string &location) {
  Response resp;

  resp.statusCode = 301;
  resp.contentType = "text/html";
  resp.headers["Location"] = location;
  resp.body = "<html><body>Moved Permanently</body></html>";
  return resp;
}

static bool tryCGI(const HTTPRequest &request, const RouteConfig &route) {
  std::string extension;
  const std::map<std::string, std::string> &cgiPass = route.getCgiPass();

  extension = extractExtention(request.getURI());
  if (extension.empty() || cgiPass.empty())
    return (false);
  return (cgiPass.count(extension));
}

// sorted dirs and files
static std::string buildAutoindex(const std::string &dirPath, const std::string &uri) {
  DIR *dir = opendir(dirPath.c_str());
  if (!dir) return "";

  std::vector<std::string> entries;
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    std::string name = entry->d_name;
    if (name == "." || name == "..") continue;
    entries.push_back(name);
  }
  closedir(dir);
  std::sort(entries.begin(), entries.end());

  std::string base = uri;
  if (base.empty() || base[base.size() - 1] != '/') base += "/";

  std::ostringstream html;
  html << "<!DOCTYPE html><html lang=\"en\"><head>"
       << "<meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
       << "<title>Index of " << uri << "</title>"
       << "<link href=\"https://fonts.googleapis.com/css2?family=Syne:wght@700&family=DM+Sans:wght@400;500&display=swap\" rel=\"stylesheet\">"
       << "<style>"
       << ":root{--bg:#04050f;--glass:rgba(255,255,255,0.05);--glass-b:rgba(255,255,255,0.1);--teal:#00f0d4;--violet:#a78bfa;--text:#e2e8f0;--muted:#94a3b8;}"
       << "body{background:var(--bg);color:var(--text);font-family:'DM Sans',sans-serif;margin:0;padding:40px;min-height:100vh;display:flex;justify-content:center;}"
       << ".aurora{position:fixed;inset:0;z-index:-1;overflow:hidden;background:#04050f;}"
       << ".blob{position:absolute;width:600px;height:600px;background:radial-gradient(circle,rgba(167,139,250,0.15),transparent);filter:blur(100px);top:-200px;left:-100px;}"
       << ".container{width:100%;max-width:900px;z-index:1;}"
       << "h1{font-family:'Syne',sans-serif;font-size:32px;margin-bottom:24px;background:linear-gradient(135deg,var(--teal),var(--violet));-webkit-background-clip:text;-webkit-text-fill-color:transparent;}"
       << ".list{background:var(--glass);border:1px solid var(--glass-b);border-radius:20px;backdrop-filter:blur(20px);overflow:hidden;}"
       << ".item{display:flex;align-items:center;padding:14px 24px;text-decoration:none;color:var(--text);border-bottom:1px solid var(--glass-b);transition:all 0.2s;}"
       << ".item:last-child{border-bottom:none;}"
       << ".item:hover{background:rgba(255,255,255,0.08);padding-left:32px;color:var(--teal);}"
       << ".icon{margin-right:16px;font-size:18px;opacity:0.7;}"
       << ".footer{margin-top:20px;font-size:12px;color:var(--muted);text-align:right;letter-spacing:1px;}"
       << "</style></head><body>"
       << "<div class=\"aurora\"><div class=\"blob\"></div></div>"
       << "<div class=\"container\">"
       << "<h1>Index of " << uri << "</h1>"
       << "<div class=\"list\">"
       << "<a href=\"../\" class=\"item\"><span class=\"icon\">↩</span> Parent Directory</a>";

  for (size_t i = 0; i < entries.size(); i++) {
    const std::string &name = entries[i];
    std::string fullPath = dirPath;
    if (fullPath[fullPath.size() - 1] != '/') fullPath += "/";
    fullPath += name;

    bool isDir = isDirectory(fullPath);
    std::string display = name + (isDir ? "/" : "");
    std::string icon = isDir ? "📁" : "📄";

    html << "<a href=\"" << base << display << "\" class=\"item\">"
         << "<span class=\"icon\">" << icon << "</span> " << display << "</a>";
  }

  html << "</div><div class=\"footer\">WEBSERV v24.0 // HTTP/1.1</div></div></body></html>";
  return html.str();
}

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

// check if allowed and safe > check if redirect > check if cgi > check if
// directory > check if file
Response MethodHandler::handleGET(HTTPRequest &request,
                                  const RouteConfig &route) {
  Response resp;
  size_t queryPos;
  HTTPRequest indexReq;

  if (isSafeAndAllowed("GET", request, route, resp))
    return applySession(request, resp);
  // check if there is redir in the config
  if (!route.getRedirect().empty())
    return applySession(request, makeRedirect(route.getRedirect()));
  // check if the request in cgi file
  if (tryCGI(request, route)) {
    request.setIsCGI(true);
    return applySession(request, resp);
  }
  std::string ourFilePath = makeThePath(route.getRoot(), request.getURI());
  // check if dir
  if (isDirectory(ourFilePath)) {
    // if it has no / at the end redirect to the same uri with / added
    std::string currentUri = RemoveQueryString(request.getURI());
    if (!currentUri.empty() && currentUri[currentUri.size() - 1] != '/')
      return applySession(request, makeRedirect(currentUri + "/"));
    // check if there is an index file in the config
    std::string indexFile = route.getIndex();
    if (!indexFile.empty()) {
      std::string indexPath = ourFilePath;
      if (indexPath[indexPath.size() - 1] != '/')
        indexPath += "/";
      indexPath += indexFile;
      if (fileExists(indexPath)) {
        std::string baseURI = request.getURI();
        queryPos = baseURI.find('?');
        std::string justPath;
        std::string queryPart;
        // get the query out of the directory URI
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
        indexReq = request;
        indexReq.setURI(indexedURI);
        if (tryCGI(indexReq, route)) {
          request.setIsCGI(true);
          return applySession(request, resp);
        }
        if (!isReadable(indexPath))
          return applySession(
              request, MethodHandler::makeError(403, "Forbidden", route));
        std::string content;
        if (!readFileContent(indexPath, content))
          return applySession(
              request,
              MethodHandler::makeError(500, "Internal Server Error", route));
        resp.statusCode = 200;
        resp.contentType = getTheFileType(indexPath);
        resp.body = content;
        return applySession(request, resp);
      }
    }
    // check if autoindex is enabled
    if (route.getAutoindex()) {
      std::string listing =
          buildAutoindex(ourFilePath, RemoveQueryString(request.getURI()));
      if (listing.empty())
        return applySession(request, MethodHandler::makeError(
                                         500, "Internal Server Error", route));
      resp.statusCode = 200;
      resp.contentType = "text/html";
      resp.body = listing;
      return applySession(request, resp);
    }
    // no index no autoindex
    return applySession(request,
                        MethodHandler::makeError(403, "Forbidden", route));
  }
  // neither dir or a file
  if (!fileExists(ourFilePath))
  {
		// std::cout << ourFilePath << std::endl; // FIXME: cannot find script who isn't in pass cgi, so i can serve them as a static files
	  return applySession(request,
		MethodHandler::makeError(404, "Not Found", route));
}
  if (!isReadable(ourFilePath))
    return applySession(request,
                        MethodHandler::makeError(403, "Forbidden", route));
  std::string content;
  if (!readFileContent(ourFilePath, content))
    return applySession(
        request, MethodHandler::makeError(500, "Internal Server Error", route));
  resp.statusCode = 200;
  resp.contentType = getTheFileType(ourFilePath);
  resp.body = content;
  return applySession(request, resp);
}

Response MethodHandler::handlePOST(HTTPRequest &request,
                                   const RouteConfig &route) {
  Response resp;
  FileUpload uploader;
  bool isSaved;

  if (isSafeAndAllowed("POST", request, route, resp))
    return applySession(request, resp);
  // redirect in config file?
  if (!route.getRedirect().empty())
    return applySession(request, makeRedirect(route.getRedirect()));
  // body size bigger than config file max body
  if (route.getMaxBodySize() > 0 &&
      request.getBody().size() > route.getMaxBodySize())
    return applySession(
        request, MethodHandler::makeError(413, "Payload Too Large", route));
  if (tryCGI(request, route)) {
    request.setIsCGI(true);
    return applySession(request, resp);
  }
  // multipart file upload
  std::string contentType = request.getHeader("content-type");
  if (contentType.find("multipart/form-data") != std::string::npos) {
    FileData file;
    if (!uploader.parseTheThing(request, file))
      return applySession(request,
                          MethodHandler::makeError(400, "Bad Request", route));
    std::string uploadDir = route.getUploadStore();
    if (uploadDir.empty()) {
      std::cerr << "MethodHandler: no upload directory configured\n";
      return applySession(request, MethodHandler::makeError(
                                       500, "Internal Server Error", route));
    }
    // upload directory exists
    if (!fileExists(uploadDir) || !isDirectory(uploadDir)) {
      std::cerr << "MethodHandler: upload directory missing: " << uploadDir
                << "\n";
      return applySession(request, MethodHandler::makeError(
                                       500, "Internal Server Error", route));
    }

    std::ostringstream res;
    res << "<!DOCTYPE html>\n<html><body>\n"
        << "<h1>Upload Complete</h1>\n<ul>\n";
    if (uploader.saveTheThing(file, uploadDir)) {
      res << "<li>" << file.filename << " - uploaded ok</li>\n";
      isSaved = true;
    } else {
      res << "<li>" << file.filename << " - FAILED</li>\n";
      isSaved = false;
    }
    res << "</ul>\n</body></html>\n";
    if (isSaved) {
      resp.statusCode = 201;
    } else {
      resp.statusCode = 500;
    }
    resp.contentType = "text/html";
    if (isSaved) {
      std::string location = RemoveQueryString(request.getURI());
      if (location.empty())
        location = "/";
      else if (location[0] != '/')
        location = "/" + location;
      resp.headers["Location"] = location;
    }
    resp.body = res.str();
    return applySession(request, resp);
  }
  //    application/x-www-form-urlencoded or raw body
  resp.statusCode = 200;
  resp.contentType = "text/html";
  resp.body = "<!DOCTYPE html>\n<html><body>"
              "<h1>200 OK</h1>"
              "<p>Request body received.</p>"
              "</body></html>\n";
  return applySession(request, resp);
}

Response MethodHandler::handleDELETE(HTTPRequest &request,
                                     const RouteConfig &route) {
  Response resp;
  if (isSafeAndAllowed("DELETE", request, route, resp))
    return applySession(request, resp);

  std::string ourFilePath = makeThePath(route.getRoot(), request.getURI());

  // if dir, not allowed
  if (isDirectory(ourFilePath))
    return applySession(request,
                        MethodHandler::makeError(403, "Forbidden", route));

  // file doesnt exxists
  if (!fileExists(ourFilePath))
    return applySession(request,
                        MethodHandler::makeError(404, "Not Found", route));
  // is it possible to delete it
  if (!canDeletePath(ourFilePath))
    return applySession(request,
                        MethodHandler::makeError(403, "Forbidden", route));

  if (std::remove(ourFilePath.c_str()) != 0) {
    std::cerr << "MethodHandler: failed to delete: " << ourFilePath << "\n";
    return applySession(
        request, MethodHandler::makeError(500, "Internal Server Error", route));
  }

  // 204 No Content – no body, no content-type
  resp.statusCode = 204;
  resp.body = "";
  return applySession(request, resp);
}