#pragma once

#include "MethodHandler.hpp"
#include "Response.hpp"
#include "RouteConfig.hpp"
#include <string>

// Serialises a Response struct into the raw bytes that go on the wire.
//
// Format produced:
//   HTTP/1.0 <code> <reason>\r\n
//   Content-Type: <type>\r\n
//   Content-Length: <len>\r\n
//   <extra headers>\r\n
//   \r\n
//   <body>

class ResponseBuilder
{
  public:
	ResponseBuilder();
	std::string buildCgiResponse(std::string cgiResult,
		RouteConfig &Route) const;
	// Build the full HTTP response string ready for send()
	std::string build(const Response &resp) const;

	// Build an error response from just a status code
	// (used by the server loop when MethodHandler isn't reached,
	//  e.g. 400 Bad Request from the parser)
	std::string buildError(int code) const;

  private:
	// Map a numeric status code to its reason phrase
	static std::string reasonPhrase(int code);
};
