#pragma once
#include <map>
#include <iostream>
class config_parser {

    public:
        std::vector<std::string> vect;
        config_parser(const char* file_path);
    };
    void parse(const char* file_path);