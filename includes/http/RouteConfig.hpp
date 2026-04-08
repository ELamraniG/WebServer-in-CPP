#pragma once

#include <string>
#include <vector>

// Temporary RouteConfig class for Person 2 to code against
// Person 3 will replace this with their real implementation
// that reads from the config file
//
// interface based on what method handlers need:
//   - getRoot() -> document root path
//   - getAllowedMethods() -> which methods are ok
//   - getUploadStore() -> where to save uploaded files
//   - getCGIPass() -> path to CGI executable (like /usr/bin/php)
//   - getMaxBodySize() -> max body size in bytes (0 = no limit)
//   - getIndex() -> default index file (like "index.html")
//   - getAutoindex() -> show directory listing or not

class RouteConfig {
private:
  std::string root;
  std::vector<std::string> allowedMethods;
  std::string uploadStore;
  std::string cgiPass;
  std::string redirect;
  size_t maxBodySize;
  std::string index;
  bool autoindex;

public:
  RouteConfig()
      : root(""), uploadStore(""), cgiPass(""), redirect(""), maxBodySize(0),
        index(""), autoindex(false) {}

  // getters
  const std::string &getRoot() const { return root; }
  const std::vector<std::string> &getAllowedMethods() const {
    return allowedMethods;
  }
  const std::string &getUploadStore() const { return uploadStore; }
  const std::string &getCGIPass() const { return cgiPass; }
  const std::string &getRedirect() const { return redirect; }
  size_t getMaxBodySize() const { return maxBodySize; }
  const std::string &getIndex() const { return index; }
  bool getAutoindex() const { return autoindex; }

  // setters - person 3 will fill these from the config parser
  void setRoot(const std::string &r) { root = r; }
  void addAllowedMethod(const std::string &m) { allowedMethods.push_back(m); }
  void setUploadStore(const std::string &u) { uploadStore = u; }
  void setCGIPass(const std::string &c) { cgiPass = c; }
  void setRedirect(const std::string &r) { redirect = r; }
  void setMaxBodySize(size_t s) { maxBodySize = s; }
  void setIndex(const std::string &i) { index = i; }
  void setAutoindex(bool a) { autoindex = a; }
};
