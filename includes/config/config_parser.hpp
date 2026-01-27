#pragma once
#include <map>
#include <iostream>
class config_parser {

    public:
        config_parser(const char* file_path);
    };
    void parse(const char* file_path);