#include "../includes/http/CGIHandler.hpp"
#include "../includes/http/RouteConfig.hpp"
#include <iostream>
#include <cstdio>

CGIHandler::CGIHandler() {}

bool CGIHandler::isCGIRequest(const std::string &uri, const RouteConfig &route) {
    (void)route;
    return uri.find(".go") != std::string::npos || uri.find(".cgi") != std::string::npos;
}

CGIResult CGIHandler::execute(const HTTPRequest &request, const RouteConfig &route) {
    (void)request;
    CGIResult result;
    result.success = false;
    result.statusCode = 500;
    
    std::string cmd = route.getCGIPass();
    if (cmd.empty()) return result;
    
    FILE *fp = popen(cmd.c_str(), "r");
    if (!fp) return result;
    
    char buffer[1024];
    std::string output;
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        output += buffer;
    }
    pclose(fp);
    
    result.success = true;
    result.statusCode = 200;
    size_t headerEnd = output.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        result.body = output.substr(headerEnd + 4);
        result.headers["content-type"] = "text/plain"; 
    } else {
        result.body = output;
    }
    return result;
}

char **CGIHandler::buildEnvironment(const HTTPRequest &request) { (void)request; return NULL; }
std::string CGIHandler::buildScriptPath(const std::string &uri, const RouteConfig &route) const { (void)uri; (void)route; return ""; }
void CGIHandler::freeEnvironment(char **envp) const { (void)envp; }
