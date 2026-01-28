#pragma once
#include <map>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

class config_parser {

    public:
        std::vector<std::string> tokens;
        config_parser(const char* file_path);
        void parse(const char* file_path);
        void tokenize(std::string s);
       // bool has_only_space(std::string s);
    };
