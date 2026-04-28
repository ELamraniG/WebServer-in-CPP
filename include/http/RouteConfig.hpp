#ifndef ROUTE_CONFIG_HPP
#define ROUTE_CONFIG_HPP

#include "../config/location_block.hpp"
#include "../config/Server_block.hpp"
#include <map>
#include <string>
#include <vector>


class RouteConfig {
private:
  const Server_block &_server;
  const location_block *_location;
  mutable std::vector<std::string> _allowed_methods_cache;

public:
  RouteConfig(const Server_block &s, const location_block *l);

  const std::string &getRoot() const;

  const std::string &getIndex() const;

  bool getAutoindex() const;

  const std::vector<std::string> &getAllowedMethods() const;

  const std::string &getRedirect() const;
  int getRedirectCode() const;

  const std::string &getUploadStore() const;

  const std::map<int, std::string> &getErrorPages() const;
  size_t getMaxBodySize() const;
  const std::map<std::string, std::string> &getCgiPass() const;

  const std::string &getLocationPath() const;

};

#endif
