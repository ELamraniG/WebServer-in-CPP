#include "../includes/http/CGIHandler.hpp"
#include "../includes/http/RouteConfig.hpp"
CGIHandler::CGIHandler() {}
bool CGIHandler::isCGIRequest(const std::string &, const RouteConfig &) { return false; }
CGIResult CGIHandler::execute(const HTTPRequest &, const RouteConfig &) { CGIResult r; r.success = false; r.statusCode = 500; r.errorMessage = "stub"; return r; }
char **CGIHandler::buildEnvironment(const HTTPRequest &) { return 0; }
std::string CGIHandler::buildScriptPath(const std::string &, const RouteConfig &) const { return ""; }
void CGIHandler::freeEnvironment(char **) const {}
