#pragma once

#include <iostream>
#include <vector>
#include <map>
class location_block {

    public:

        std::string root;
        std::string index;
        std::vector<std::string> allowed_methods;
        bool autoindex;
        std::string upload_pass;
        std::map<std::string, std::string> cgi_pass;
        
    public:
        location_block();
        ~location_block();

};