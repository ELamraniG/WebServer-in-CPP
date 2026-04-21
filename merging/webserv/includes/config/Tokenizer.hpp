#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

class Tokenizer {

    public:
        std::vector<std::string> tokens;
        Tokenizer(const char* file_path);
        void parse(const char* file_path);
        void tokenize(std::string s);
       // bool has_only_space(std::string s);
    };
