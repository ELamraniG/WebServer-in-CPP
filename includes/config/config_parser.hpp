#pragma once
#include <map>
#include <iostream>
#include <ifstream>
class config_parser {

    public:
        std::vector<std::string> vect;
        config_parser(const char* file_path);
    };
    bool parse(const char* file_path, std::vector<std::string> &vect);
    bool tokenize(std::ifstream &file, std::vector<std::string> &vect);
    bool has_only_space(std::string s);