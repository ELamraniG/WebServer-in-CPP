#pragma once

#include <iostream>
#include <vector>
#include <map>
class location_block {

    public:

        std::string root;
        std::string path;
        std::string index;
        std::map<int, std::string> redirect;
      	std::map<std::string, bool> methods;
        bool autoindex;
        std::string upload_pass;
        std::map<std::string, std::string> cgi_pass;
        
    public:
        location_block();
        ~location_block();

};