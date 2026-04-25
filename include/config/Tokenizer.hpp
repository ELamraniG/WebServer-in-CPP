#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

class Tokenizer {

    public:
        std::vector<std::string> tokens;
        Tokenizer(std::string file_path);
		Tokenizer(const char *s);
        void parse(const char *file_path);
        void tokenize(std::string s);
       // bool has_only_space(std::string s);
    };
