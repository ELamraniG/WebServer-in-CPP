#pragma once
//IMPORTANT NOTE: THIS IS NOT MY TASK ONLY CREATED THIS BECAUSE I NEED IT TO TEST MY OTHER TASK
#include <string>
#include <map>
#include <iostream>

struct Request {
    std::string method;                         // "GET", "POST", etc.
    std::string uri;                            // "/images/logo.png"
    std::string http_version;                   // "HTTP/1.1"
    std::map<std::string, std::string> headers; // "Host" -> "localhost"
    std::string body;                           // Content of POST

    Request(std::string m, std::string u) : method(m), uri(u) {}
};

