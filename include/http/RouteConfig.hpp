#ifndef ROUTE_CONFIG_HPP
#define ROUTE_CONFIG_HPP

#include "../config/location_block.hpp"
#include "../config/Server_block.hpp"
#include <map>
#include <string>
#include <vector>

// Adapter: presents a Server_block + matched location_block as the flat
// interface that MethodHandler expects. Handles format mismatches:
// - methods: map<string,bool> -> vector<string> (only keys where true)
// - redirect: map<int,string> -> single (code, url) pair
class RouteConfig {
private:
  const Server_block &_server;
  const location_block *_location;
  mutable std::vector<std::string> _allowed_methods_cache;
  mutable bool _cache_valid;

public:
  RouteConfig(const Server_block &s, const location_block *l);

  // Get root: location root if set, else server root
  const std::string &getRoot() const;

  // Get index file: location index if set, else server index
  const std::string &getIndex() const;

  // Get autoindex setting from location (or false if no location)
  bool getAutoindex() const;

  // Get allowed methods as vector<string> (flattened from location.methods map)
  const std::vector<std::string> &getAllowedMethods() const;

  // Get redirect URL (from location.redirect, or empty if none)
  const std::string &getRedirect() const;

  // Get redirect HTTP code (from location.redirect, or 0 if none)
  int getRedirectCode() const;

  // Get upload store directory (from location.upload_pass)
  const std::string &getUploadStore() const;

  // Get error pages map (from server.error_pages)
  const std::map<int, std::string> &getErrorPages() const;

  // Get max body size (from server.client_max_body_size)
  size_t getMaxBodySize() const;

  // Get CGI pass config (from location.cgi_pass)
  const std::map<std::string, std::string> &getCgiPass() const;

  // Get the matched location path (for logging/debugging)
  const std::string &getLocationPath() const;
};

#endif
