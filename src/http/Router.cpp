#include "../../include/http/Router.hpp"
#include <cstddef>
#include <string>

// Helper: is `location_path` a prefix of `uri_path`?
static bool is_prefix(const std::string &uri_path,
                      const std::string &location_path) {
  if (location_path.length() > uri_path.length())
    return false;
  return uri_path.compare(0, location_path.length(), location_path) == 0;
}

// Helper: after a prefix match, is the match on a path-segment boundary?
// Matches: loc="/api"   uri="/api"     (equal)
// Matches: loc="/api"   uri="/api/foo" (next char is '/')
// Matches: loc="/api/"  uri="/api/foo" (loc already ends with '/')
// Matches: loc="/"      uri="/anything"
// Skips:   loc="/api"   uri="/apis"    (next char is 's')
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

// -------- match_server ---------------------------------------------------
// Public API kept identical to your original to avoid breaking callers.
// If you know the actual local listen address:port the connection came in on,
// prefer match_server(req, servers, local_host, local_port) below — that is
// the RFC-correct route, independent of what the client puts in the Host
// header.
Server_block &Router::match_server(const HTTPRequest &req,
                                   std::vector<Server_block> &servers) {
  // Safety: parser should reject empty configs, but never dereference [0]
  // blindly. If the caller ever hands us an empty vector, that's a bug we
  // report via a thrown error rather than a crash.
  if (servers.empty())
    throw std::runtime_error("Router::match_server: no servers configured");

  std::string host_header = req.getHeader("host");

  // No Host header -> fall back to first server (HTTP/1.0 clients may omit it)
  if (host_header.empty())
    return servers[0];

  // Split "hostname[:port]"
  std::string hostname = host_header;
  std::string port = "";
  std::size_t colon_pos = host_header.find(':');
  if (colon_pos != std::string::npos) {
    hostname = host_header.substr(0, colon_pos);
    port = host_header.substr(colon_pos + 1);
  }

  // Priority:
  //   1. exact match on host AND port (only meaningful if user bound to a
  //      specific interface AND the Host header carries a port)
  //   2. first server listening on the requested port
  //   3. first server overall
  Server_block *port_match = NULL;
  for (std::size_t i = 0; i < servers.size(); ++i) {
    if (!port.empty() && servers[i].port == port) {
      if (port_match == NULL)
        port_match = &servers[i];
      if (hostname == servers[i].host)
        return servers[i];
    }
  }
  if (port_match != NULL)
    return *port_match;
  return servers[0];
}

// Overload: use this from your connection layer where you DO know what
// local host:port the socket was bound to. This is the correct route when
// virtual hosts are out of scope (every server block has a unique bind).
Server_block &Router::match_server(const HTTPRequest & /*req*/,
                                   std::vector<Server_block> &servers,
                                   const std::string &local_host,
                                   const std::string &local_port) {
  if (servers.empty())
    throw std::runtime_error("Router::match_server: no servers configured");

  Server_block *port_match = NULL;
  for (std::size_t i = 0; i < servers.size(); ++i) {
    if (servers[i].port != local_port)
      continue;
    if (port_match == NULL)
      port_match = &servers[i];
    // Exact host match wins; "0.0.0.0" is the wildcard default
    if (servers[i].host == local_host || servers[i].host == "0.0.0.0")
      return servers[i];
  }
  if (port_match != NULL)
    return *port_match;
  return servers[0];
}

// -------- match_location -------------------------------------------------
location_block *Router::match_location(const HTTPRequest &req,
                                       Server_block &server) {
  // Strip query string: "/path?foo=bar" -> "/path"
  std::string uri = req.getURI();
  std::size_t query_pos = uri.find('?');
  if (query_pos != std::string::npos)
    uri = uri.substr(0, query_pos);

  location_block *best_loc = NULL;
  std::size_t best_len = 0;

  for (std::size_t i = 0; i < server.locations.size(); ++i) {
    location_block *loc = &server.locations[i];

    if (!is_prefix(uri, loc->path))
      continue;

    // Bug fix: the old version mis-handled trailing slashes in loc->path.
    // "/api/" vs "/api/foo" must match; the old code skipped it.
    if (!is_segment_boundary(uri, loc->path))
      continue;

    // Longest-prefix wins
    if (loc->path.length() > best_len) {
      best_len = loc->path.length();
      best_loc = loc;
    }
  }
  return best_loc;
}
