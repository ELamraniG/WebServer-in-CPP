#pragma once
#include <map>

class config_parser {
    private:
        std::map<std::string, std::string> config_map;
    public:
        config_parser(const char* file_path);
    };
    void parse(const char* file_path, std::map<std::string, std::string> &config_map);