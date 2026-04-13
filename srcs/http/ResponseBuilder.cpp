#include "../../includes/http/ResponseBuilder.hpp"

#include <ctime>
#include <sstream>

ResponseBuilder::ResponseBuilder() {}

// ─────────────────────────────────────────────
//  Reason phrases (covers every code the server can produce)
// ─────────────────────────────────────────────

std::string ResponseBuilder::reasonPhrase(int code) {
  switch (code) {
  // 2xx
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 204:
    return "No Content";

  // 3xx
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 307:
    return "Temporary Redirect";
  case 308:
    return "Permanent Redirect";

  // 4xx
  case 400:
    return "Bad Request";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 408:
    return "Request Timeout";
  case 413:
    return "Payload Too Large";
  case 414:
    return "URI Too Long";

  // 5xx
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  case 502:
    return "Bad Gateway";
  case 504:
    return "Gateway Timeout";
  case 505:
    return "HTTP Version Not Supported";

  default: {
    // Fallback: produce a generic reason from the class
    if (code >= 200 && code < 300)
      return "OK";
    if (code >= 300 && code < 400)
      return "Redirection";
    if (code >= 400 && code < 500)
      return "Client Error";
    return "Server Error";
  }
  }
}

// ─────────────────────────────────────────────
//  Date header (RFC 7231 §7.1.1.2)
//  HTTP/1.0 – Date header is good practice for any origin server.
// ─────────────────────────────────────────────

static std::string httpDate() {
  char buf[64];
  std::time_t now = std::time(NULL);
  struct std::tm *gmt = std::gmtime(&now);
  // Sun, 06 Nov 1994 08:49:37 GMT
  std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
  return std::string(buf);
}

// ─────────────────────────────────────────────
//  build()  –  serialise a Response into raw HTTP bytes
// ─────────────────────────────────────────────

std::string ResponseBuilder::build(const Response &resp) const {
  std::ostringstream out;

  // --- Status line ---
  out << "HTTP/1.0 " << resp.statusCode << " " << reasonPhrase(resp.statusCode)
      << "\r\n";

  // --- Date header ---
  out << "Date: " << httpDate() << "\r\n";

  // --- Server header (42 subject says we can pick a name) ---
  out << "Server: webserv/1.0\r\n";

  // --- Extra headers from the Response struct ---
  // (Location, Set-Cookie, etc. added by MethodHandler / CGI)
  for (std::map<std::string, std::string>::const_iterator it =
           resp.headers.begin();
       it != resp.headers.end(); ++it) {
    // Skip content-type / content-length because we set those below
    std::string key = it->first;
    // lowercase compare
    std::string lower;
    for (std::string::size_type i = 0; i < key.size(); ++i)
      lower +=
          static_cast<char>(std::tolower(static_cast<unsigned char>(key[i])));
    if (lower == "content-type" || lower == "content-length")
      continue;
    out << it->first << ": " << it->second << "\r\n";
  }

  // --- Content-Type and Content-Length ---
  // 204 / 304 responses MUST NOT include a body or Content-Type
  if (resp.statusCode != 204 && resp.statusCode != 304) {
    if (!resp.contentType.empty())
      out << "Content-Type: " << resp.contentType << "\r\n";

    out << "Content-Length: " << resp.body.size() << "\r\n";
  }

  // --- Connection header ---
  // We always announce the connection mode so clients know what to expect.
  out << "Connection: close\r\n";

  // --- End of headers ---
  out << "\r\n";

  // --- Body ---
  if (resp.statusCode != 204 && resp.statusCode != 304)
    out << resp.body;

  return out.str();
}

// ─────────────────────────────────────────────
//  buildError()  –  standalone error page
//  Used when the server loop needs to reject a request
//  before MethodHandler is reached (bad parse, timeout, etc.)
// ─────────────────────────────────────────────

std::string ResponseBuilder::buildError(int code) const {
  Response resp;
  resp.statusCode = code;
  resp.contentType = "text/html";

  std::string reason = reasonPhrase(code);
  std::ostringstream body;
  body << "<!DOCTYPE html>\n<html><head><title>" << code << " " << reason
       << "</title></head>\n<body><h1>" << code << " " << reason
       << "</h1></body></html>\n";
  resp.body = body.str();

  return build(resp);
}
