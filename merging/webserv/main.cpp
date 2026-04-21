#include <iostream>
#include <string>
#include <vector>

#include "includes/config/Parser.hpp"
#include "includes/config/Tokenizer.hpp"
#include "includes/http/HTTPRequest.hpp"
#include "includes/http/MethodHandler.hpp"
#include "includes/http/RequestParser.hpp"
#include "includes/http/ResponseBuilder.hpp"
#include "includes/http/RouteConfig.hpp"
#include "includes/http/Router.hpp"

// Test harness: reads a config file, parses it, then processes a hardcoded HTTP
// request This is NOT the final server loop (Person 1 will replace with
// socket/poll). It's a smoke test to verify the merge: Tokenizer → Parser →
// Router → MethodHandler → ResponseBuilder.

int main() {
  try {
    // Step 1: Parse config file
    std::cout << "=== Step 1: Parsing config ===" << std::endl;
    Tokenizer tokenizer("config/config.conf");
    Parser parser(tokenizer.tokens);
    parser.parse();
    std::vector<server_block> servers = parser.getServers();
    std::cout << "Parsed " << servers.size() << " server block(s)" << std::endl;
    for (size_t i = 0; i < servers.size(); i++) {
      std::cout << "  [" << i << "] host=" << servers[i].host
                << " port=" << servers[i].port
                << " locations=" << servers[i].locations.size() << std::endl;
    }

    // Step 2: Create a test HTTP request
    std::cout << "\n=== Step 2: Parsing HTTP request ===" << std::endl;
    // Test a redirect
    std::string raw_request = "GET /old HTTP/1.1\r\n"
                              "Host: 127.0.0.1:80\r\n"
                              "Content-Length: 0\r\n"
                              "\r\n";
    std::cout << "Request:\n" << raw_request << std::endl;

    // Step 3: Parse HTTP request
    RequestParser req_parser;
    HTTPRequest http_req;
    RequestParser::ParsingStatus status =
        req_parser.parseRequest(raw_request, http_req);
    std::cout << "Parse status: " << (int)status
              << " (0=incomplete, 1=success, 2=error)" << std::endl;

    if (status != RequestParser::P_SUCCESS) {
      std::cerr << "Request parsing failed!" << std::endl;
      return 1;
    }

    std::cout << "  Method: " << http_req.getMethod() << std::endl;
    std::cout << "  URI: " << http_req.getURI() << std::endl;
    std::cout << "  Host: " << http_req.getHeader("host") << std::endl;

    // Step 4: Route the request
    std::cout << "\n=== Step 3: Routing ===" << std::endl;
    if (servers.empty()) {
      std::cerr << "No servers parsed from config!" << std::endl;
      return 1;
    }
    server_block &matched_server = Router::match_server(http_req, servers);
    std::cout << "Matched server: " << matched_server.host << ":"
              << matched_server.port << std::endl;

    location_block *matched_location =
        Router::match_location(http_req, matched_server);
    std::cout << "Matched location: "
              << (matched_location ? matched_location->path : "(root)")
              << std::endl;

    // Step 5: Build RouteConfig adapter
    std::cout << "\n=== Step 4: Building RouteConfig ===" << std::endl;
    RouteConfig route(matched_server, matched_location);
    std::cout << "Root: " << route.getRoot() << std::endl;
    std::cout << "Index: " << route.getIndex() << std::endl;
    std::cout << "Autoindex: " << (route.getAutoindex() ? "on" : "off")
              << std::endl;

    // Step 6: Handle the request
    std::cout << "\n=== Step 5: Handling request ===" << std::endl;
    MethodHandler handler;
    Response resp;
    std::string method = http_req.getMethod();
    if (method == "GET") {
      resp = handler.handleGET(http_req, route);
    } else if (method == "POST") {
      resp = handler.handlePOST(http_req, route);
    } else if (method == "DELETE") {
      resp = handler.handleDELETE(http_req, route);
    } else {
      resp.statusCode = 501;
      resp.body = "Method not implemented";
    }

    std::cout << "Response status: " << resp.statusCode << std::endl;
    std::cout << "Response body size: " << resp.body.size() << " bytes"
              << std::endl;

    // Step 7: Build wire response
    std::cout << "\n=== Step 6: Building wire response ===" << std::endl;
    ResponseBuilder builder;
    std::string wire_response = builder.build(resp);
    std::cout << "Wire response size: " << wire_response.size() << " bytes"
              << std::endl;

    // Step 8: Output
    std::cout << "\n=== Final Response ===" << std::endl;
    std::cout << wire_response << std::endl;

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
