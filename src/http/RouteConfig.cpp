#include "../../include/http/RouteConfig.hpp"

RouteConfig::RouteConfig(const Server_block &s, const location_block *l)
    : _server(s), _location(l), _cache_valid(false) {}

// Root: location overrides server if it set one.
// Subject example: URL /kapouet rooted to /tmp/www ; URL /kapouet/pouic/toto
// searches /tmp/www/pouic/toto. Callers achieve this by computing
//     file_path = getRoot() + (uri.substr(getLocationPath().length()))
const std::string &RouteConfig::getRoot() const {
  if (_location && !_location->root.empty())
    return _location->root;
  return _server.root;
}

const std::string &RouteConfig::getIndex() const {
  if (_location && !_location->index.empty())
    return _location->index;
  return _server.index;
}

bool RouteConfig::getAutoindex() const {
  if (_location)
    return _location->autoindex;
  return false;
}

const std::vector<std::string> &RouteConfig::getAllowedMethods() const {
  if (!_cache_valid) {
    _allowed_methods_cache.clear();
    if (_location) {
      for (std::map<std::string, bool>::const_iterator it =
               _location->methods.begin();
           it != _location->methods.end(); ++it) {
        if (it->second)
          _allowed_methods_cache.push_back(it->first);
      }
    }
    // Contract: if no allowed_methods directive was given in the config for
    // this location, the cache stays empty. Your request handler must decide
    // whether that means "allow all" (NGINX default) or "deny all". The
    // evaluation sheet explicitly tests rejecting DELETE without permission,
    // so make sure your config always sets allowed_methods for routes where
    // you care.
    _cache_valid = true;
  }
  return _allowed_methods_cache;
}

const std::string &RouteConfig::getRedirect() const {
  static const std::string empty_str;
  if (_location && !_location->redirect.empty())
    return _location->redirect.begin()->second;
  return empty_str;
}

int RouteConfig::getRedirectCode() const {
  if (_location && !_location->redirect.empty())
    return _location->redirect.begin()->first;
  return 0;
}

const std::string& RouteConfig::getLocationRoot() const {
	static const std::string empty_str;
	if (_location)
		return _location->root;
	return empty_str;
}

const std::string &RouteConfig::getUploadStore() const {
  static const std::string empty_str;
  if (_location)
    return _location->upload_pass;
  return empty_str;
}

const std::map<int, std::string> &RouteConfig::getErrorPages() const {
  return _server.error_pages;
}

size_t RouteConfig::getMaxBodySize() const {
  return _server.client_max_body_size;
}

const std::map<std::string, std::string> &RouteConfig::getCgiPass() const {
  static const std::map<std::string, std::string> empty_map;
  if (_location)
    return _location->cgi_pass;
  return empty_map;
}

const std::string &RouteConfig::getLocationPath() const {
  static const std::string empty_str;
  if (_location)
    return _location->path;
  return empty_str;
}
