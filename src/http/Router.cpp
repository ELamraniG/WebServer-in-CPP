#include "../../include/http/Router.hpp"
#include <cstddef>
#include <string>

static bool is_prefix(const std::string &uri_path,
                      const std::string &location_path) {
  if (location_path.length() > uri_path.length())
    return false;
  return uri_path.compare(0, location_path.length(), location_path) == 0;
}


static bool is_segment_boundary(const std::string &uri,
                                const std::string &loc_path) {
  if (uri.length() == loc_path.length())
    return true;
  if (loc_path == "/")
    return true;
  if (loc_path[loc_path.length() - 1] == '/')
    return true;
  return uri[loc_path.length()] == '/';
}

const location_block *Router::match_location(const HTTPRequest &req,
                                       const Server_block &server) {
  std::string uri = req.getURI();
  std::size_t query_pos = uri.find('?');
  if (query_pos != std::string::npos)
    uri = uri.substr(0, query_pos);

  const location_block *best_loc = NULL;
  std::size_t best_len = 0;

  for (std::size_t i = 0; i < server.locations.size(); ++i) {
    const location_block *loc = &server.locations[i];

    if (!is_prefix(uri, loc->path))
      continue;

    if (!is_segment_boundary(uri, loc->path))
      continue;

    if (loc->path.length() > best_len) {
      best_len = loc->path.length();
      best_loc = loc;
    }
  }
  return best_loc;
}
