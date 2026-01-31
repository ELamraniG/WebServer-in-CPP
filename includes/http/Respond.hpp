#pragma once
#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
class Respond {
    private:
    int _status;
    std::map<std::string, std::string> _headers;
    std::string _body;
    
    public:
        Respond();
        ~Respond();
        void set_status(int status);
        void set_header(const std::string& key, const std::string& value);
        void set_body(const std::string& body);
         Respond generate_response(std::string &path, std::string& error_path, bool autoindex, std::string index, std::string uri);

};