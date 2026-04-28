#pragma once

#include "MethodHandler.hpp"
#include "Response.hpp"
#include "RouteConfig.hpp"
#include <string>


class ResponseBuilder
{
  public:
	ResponseBuilder();
	std::string buildCgiResponse(std::string cgiResult,
		RouteConfig &Route) const;
	std::string build(const Response &resp) const;
	std::string buildError(int code) const;

  private:
	static std::string reasonPhrase(int code);
};
