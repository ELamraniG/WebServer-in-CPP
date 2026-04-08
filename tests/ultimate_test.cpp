#include "../includes/http/RequestParser.hpp"
#include "../includes/http/HTTPRequest.hpp"
#include "../includes/http/MethodHandler.hpp"
#include "../includes/http/ResponseBuilder.hpp"
#include "../includes/http/ChunksDecoding.hpp"
#include "../includes/http/RouteConfig.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cassert>

void writeDummyFile(const std::string &path, const std::string &content) {
    std::ofstream out(path.c_str());
    out << content;
    out.close();
}

int main() {
    std::cout << "--- STARTING ULTIMATE TEST ---\n\n";

    // 1. Test RequestParser
    std::cout << "[1] Testing RequestParser...\n";
    RequestParser parser;
    HTTPRequest req1;
    std::string rawReq = "GET /index.html HTTP/1.0\r\nHost: localhost\r\n\r\n";
    parser.parseRequest(rawReq, req1);
    assert(req1.getMethod() == "GET");
    assert(req1.getURI() == "/index.html");
    std::cout << "    [OK] GET Request parsed successfully\n";

    // 2. Test ChunksDecoding
    std::cout << "[2] Testing ChunksDecoding...\n";
    ChunksDecoding decoder;
    std::string chunkedBody = "4\r\nWiki\r\n5\r\npedia\r\nE\r\n in\r\n\r\nchunks.\r\n0\r\n\r\n";
    std::string decodedStr;
    DecodeResult res = decoder.decode(chunkedBody, decodedStr);
    assert(res == DECODE_COMPLETE);
    assert(decodedStr == "Wikipedia in\r\n\r\nchunks.");
    std::cout << "    [OK] Chunked data decoded successfully\n";

    // Setup MethodHandler and RouteConfig
    RouteConfig route;
    route.setRoot("tests/docroot");
    route.addAllowedMethod("GET");
    route.addAllowedMethod("POST");
    route.addAllowedMethod("DELETE");
    route.setUploadStore("tests/upload");
    route.setCGIPass("/bin/bash tests/temp.sh");
    
    // Create necessary directories and dummy files
    system("mkdir -p tests/docroot tests/upload");
    writeDummyFile("tests/docroot/index.html", "<html><h1>Hello</h1></html>");
    writeDummyFile("tests/docroot/todelete.txt", "Delete me");

    MethodHandler handler;
    ResponseBuilder builder;

    // 3. Test handleGET
    std::cout << "[3] Testing MethodHandler::handleGET...\n";
    Response respGet = handler.handleGET(req1, route);
    assert(respGet.statusCode == 200);
    assert(respGet.body == "<html><h1>Hello</h1></html>");
    std::cout << "    [OK] GET handled successfully\n";
    // 3.1 Test URL Decoding
    std::cout << "[3.1] Testing URL Decoding...\n";
    writeDummyFile("tests/docroot/my page.html", "Spaces!");
    HTTPRequest reqSpace;
    parser.parseRequest("GET /my%20page.html HTTP/1.0\r\nHost: localhost\r\n\r\n", reqSpace);
    Response respSpace = handler.handleGET(reqSpace, route);
    assert(respSpace.statusCode == 200);
    assert(respSpace.body == "Spaces!");
    std::cout << "    [OK] URL Decoding handled successfully\n";

    // 3.2 Test Tricky MIME Type Directory Bug
    std::cout << "[3.2] Testing Tricky MIME Type Bug...\n";
    system("mkdir -p tests/docroot/my.folder");
    writeDummyFile("tests/docroot/my.folder/file_without_extension", "Data");
    HTTPRequest reqMime;
    parser.parseRequest("GET /my.folder/file_without_extension HTTP/1.0\r\nHost: localhost\r\n\r\n", reqMime);
    Response respMime = handler.handleGET(reqMime, route);
    assert(respMime.statusCode == 200);
    assert(respMime.contentType == "application/octet-stream");
    std::cout << "    [OK] Tricky MIME Type directory bug handled successfully\n";

    // 4. Test CGI via GET
    std::cout << "[4] Testing CGI (go run ...)...\n";
    HTTPRequest reqCGI;
    parser.parseRequest("GET /script.go HTTP/1.0\r\nHost: localhost\r\n\r\n", reqCGI);
    Response respCGI = handler.handleGET(reqCGI, route);
    assert(respCGI.statusCode == 200);
    assert(respCGI.body == "Hello from Go CGI script!\n");
    std::cout << "    [OK] CGI handled successfully\n";

    // 5. Test handleDELETE
    std::cout << "[5] Testing MethodHandler::handleDELETE...\n";
    HTTPRequest reqDel;
    parser.parseRequest("DELETE /todelete.txt HTTP/1.0\r\nHost: localhost\r\n\r\n", reqDel);
    Response respDel = handler.handleDELETE(reqDel, route);
    assert(respDel.statusCode == 204);
    std::ifstream inFile("tests/docroot/todelete.txt");
    assert(!inFile.is_open()); // File should be deleted
    std::cout << "    [OK] DELETE handled successfully\n";

    // 6. Test ResponseBuilder
    std::cout << "[6] Testing ResponseBuilder...\n";
    std::string finalResp = builder.build(respGet);
    assert(finalResp.find("HTTP/1.0 200 OK") != std::string::npos);
    assert(finalResp.find("<html><h1>Hello</h1></html>") != std::string::npos);
    std::cout << "    [OK] Response built successfully\n";

    // Clean up
    system("rm -rf tests/docroot");

    std::cout << "\n--- ALL TESTS PASSED! ---\n";
    return 0;
}
