#include "../../includes/http/RouteConfig.hpp"

RouteConfig::RouteConfig(const server_block &s, const location_block *l)
    : _server(s), _location(l), _cache_valid(false) {}

const std::string &RouteConfig::getRoot() const {
  if (_location && !_location->root.empty()) {
    return _location->root;
  }
  return _server.root;
}

const std::string &RouteConfig::getIndex() const {
  if (_location && !_location->index.empty()) {
    return _location->index;
  }
  return _server.index;
}

bool RouteConfig::getAutoindex() const {
  if (_location) {
    return _location->autoindex;
  }
  return false;
}

const std::vector<std::string> &RouteConfig::getAllowedMethods() const {
  if (!_cache_valid) {
    _allowed_methods_cache.clear();
    if (_location) {
      // Flatten location.methods map<string,bool> -> vector<string> (only true
      // keys)
      for (std::map<std::string, bool>::const_iterator it =
               _location->methods.begin();
           it != _location->methods.end(); ++it) {
        if (it->second) {
          _allowed_methods_cache.push_back(it->first);
        }
      }
    }
    _cache_valid = true;
  }
  return _allowed_methods_cache;
}

const std::string &RouteConfig::getRedirect() const {
  static const std::string empty_str;
  if (_location && !_location->redirect.empty()) {
    // redirect is map<int,string>; take the first (guaranteed <= 1 entry by
    // parser)
    return _location->redirect.begin()->second;
  }
  return empty_str;
}

int RouteConfig::getRedirectCode() const {
  if (_location && !_location->redirect.empty()) {
    return _location->redirect.begin()->first;
  }
  return 0;
}

const std::string &RouteConfig::getUploadStore() const {
  if (_location) {
    return _location->upload_pass;
  }
  static const std::string empty_str;
  return empty_str;
}

const std::map<int, std::string> &RouteConfig::getErrorPages() const {
  return _server.error_pages;
}

size_t RouteConfig::getMaxBodySize() const {
  return _server.client_max_body_size;
}

const std::map<std::string, std::string> &RouteConfig::getCgiPass() const {
  if (_location) {
    return _location->cgi_pass;
  }
  static const std::map<std::string, std::string> empty_map;
  return empty_map;
}

const std::string &RouteConfig::getLocationPath() const {
  if (_location) {
    return _location->path;
  }
  static const std::string empty_str;
  return empty_str;
}
