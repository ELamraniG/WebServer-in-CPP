#include "../../includes/http/Router.hpp"
#include <string>

// Helper: does prefix of s1 match s2? (i.e., s2 starts with s1 as a prefix)
static bool find_longest_match(const std::string &uri_path,
                               const std::string &location_path) {
  if (location_path.length() > uri_path.length())
    return false;
  return uri_path.compare(0, location_path.length(), location_path) == 0;
}

server_block &Router::match_server(const HTTPRequest &req,
                                   std::vector<server_block> &servers) {
  // Extract host and port from Host header (HTTPRequest stores all headers
  // lowercased)
  std::string host = req.getHeader("host");
  if (host.empty()) {
    // No Host header; return first server as default
    return servers[0];
  }

  std::string hostname = host;
  std::string port = "80";
  size_t colon_pos = host.find(':');
  if (colon_pos != std::string::npos) {
    port = host.substr(colon_pos + 1);
    hostname = host.substr(0, colon_pos);
  }

  server_block *default_server = NULL;
  for (size_t i = 0; i < servers.size(); i++) {
    if (servers[i].port == port) {
      if (default_server == NULL)
        default_server = &servers[i];
      if (hostname == servers[i].host)
        return servers[i];
    }
  }

  // Return matching port's default, or first server
  if (default_server != NULL)
    return *default_server;
  return servers[0];
}

location_block *Router::match_location(const HTTPRequest &req,
                                       server_block &server) {
  // Extract URI without query string (e.g., "/path/to/file?foo=bar" ->
  // "/path/to/file")
  std::string uri = req.getURI();
  size_t query_pos = uri.find('?');
  if (query_pos != std::string::npos) {
    uri = uri.substr(0, query_pos);
  }

  location_block *best_loc = NULL;
  size_t best_len = 0;

  for (size_t i = 0; i < server.locations.size(); i++) {
    location_block *loc = &server.locations[i];

    // Check if URI matches this location's path
    if (find_longest_match(uri, loc->path)) {
      // If URI is longer than location path, ensure it's a proper directory
      // boundary
      if (uri.length() > loc->path.length()) {
        // Avoid /images matching /images_backup
        if (uri[loc->path.length()] != '/' && loc->path != "/") {
          continue;
        }
      }

      // Track the longest matching location path
      if (loc->path.length() > best_len) {
        best_len = loc->path.length();
        best_loc = loc;
      }
    }
  }

  return best_loc;
}
