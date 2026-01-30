
#include "../../includes/config/Parser.hpp"
#include <exception>
#include <stdexcept>

Parser::Parser(std::vector<std::string>& tokens) : _tokens(tokens), state(GLOBAL), current_server(NULL), current_location(NULL) {}

Parser::~Parser() {}

std::vector<server_block> Parser::getServers() const {
    return servers;
}
    void Parser::parse_server_block(size_t &i)
    {
        std::string key = _tokens[i];

        if(key == "listen")
        {
            if(i + 1 >= _tokens.size() || _tokens[i + 2] != ";")
                    throw std::runtime_error("missing semicolon after 'listen'");
            if(isvalidport(_tokens[i + 1]))
                current_server->port = _tokens[i + 1];
            else
                throw std::runtime_error("invalid port number");
            i += 2;

        }
        else if(key == "error_page")
        {
            if(i + 1 >= _tokens.size() || _tokens[i + 3] != ";")
                    throw std::runtime_error("missing semicolon after 'error_page'");
            int error = isvalid_error_number(_tokens[i + 1]);
            std::cout<<error;
            if(error != -1)
                current_server->error_pages[error] = _tokens[i + 2]; 
            else
                throw std::runtime_error("invalid error page number");
            i += 3;
        }
        else if(key == "client_max_body_size")
        {
            if(i + 1 >= _tokens.size() || _tokens[i + 2] != ";")
                    throw std::runtime_error("missing semicolon after 'client_max_body_size'");
            unsigned long client_num = isvalid_client_number(_tokens[i + 1]);
             if(client_num != -1)
                current_server->client_max_body_size = client_num;
            i += 2;
        }
        else if (key == "root")
        {
            if(i + 1 >= _tokens.size() || _tokens[i + 2] != ";")
                    throw std::runtime_error("missing semicolon after 'root'");
            current_server->root = _tokens[i + 1];
            i += 2;
        }
        else if (key == "index")
        {
            if(i + 1 >= _tokens.size() || _tokens[i + 2] != ";")
                    throw std::runtime_error("missing semicolon after 'index'");
            current_server->index = _tokens[i + 1];
            i += 2;
        }
        else if(key == "host")
        {
            if(i + 1 >= _tokens.size() || _tokens[i + 2] != ";")
                    throw std::runtime_error("missing semicolon after 'host'");
            std::string ip;
            ip = _tokens[i + 1];
            if(ip == "localhost")
                ip = "127.0.0.1";
            current_server->host = ip;
            i += 2;
        }
        else
        {
            std::cout<<key<<std::endl;
            throw std::runtime_error("uknown token");
        }
       

    }
     void Parser::parse_location_block(size_t &i)
    {
        std::string key = _tokens[i];

            if(key == "root")
            {

                if(i + 1 >= _tokens.size() || _tokens[i + 2] != ";")
                        throw std::runtime_error("root in location missing value or semicolom");
                    current_location->root = _tokens[i + 1];
                i += 2;
            }
            else if(key == "index")
            {

                if(i + 1 >= _tokens.size() || _tokens[i + 2] != ";")
                        throw std::runtime_error("index in location missing value or semicolom");
                    current_location->index = _tokens[i + 1];
                i += 2;
            }
            else if(key == "autoindex")
            {

                if(i + 1 >= _tokens.size() || _tokens[i + 2] != ";")
                        throw std::runtime_error("autoindex in location missing value or semicolom");
                    if(_tokens[i + 1] == "on")
                        current_location->autoindex = true;
                    else if(_tokens[i + 1] == "off")
                        current_location->autoindex = false;
                    else
                        throw std::runtime_error("unknown token");
                i += 2;
            }
            else if(key == "allowed_methods")
            {

                    while( i + 1 < _tokens.size() && _tokens[i + 1] != ";")
                    {
                        if(_tokens[i + 1] == "GET")
                            current_location->GET = true;
                        else if(_tokens[i + 1] == "POST" && (current_location->path == "/uploads" || current_location->path == "/bin-cgi"))
                            current_location->POST = true;
                        else if(_tokens[i + 1] == "DELETE" && current_location->path == "/uploads")
                            current_location->DELETE = true;
                        else
                            throw std::runtime_error("unknown method");
                        i++;
                    }
                    if(i + 1>= _tokens.size() || _tokens[i + 1] != ";")
                        throw std::runtime_error("allowed_methods in location missing value or semicolom");
                i+=1;
            }
            else if(key == "upload_pass" && current_location->path == "/uploads")
            {
                 if(i + 1 >= _tokens.size() || _tokens[i + 2] != ";")
                        throw std::runtime_error("upload_pass in location missing value or semicolom");
                current_location->upload_pass = _tokens[i + 1];
                i += 2;
            }
            else if(key == "cgi_pass" && current_location->path == "/bin-cgi")
            {
                 if(i + 1 >= _tokens.size() || _tokens[i + 3] != ";")
                        throw std::runtime_error("cgi_pass in location missing value or semicolom");
                    std::string exetenction = _tokens[i + 1];
                if(exetenction[0] != '.')
                    throw std::runtime_error("CGI extenstion needs to start with '.'");
                current_location->cgi_pass[exetenction] = _tokens[i + 2];
                i += 2;
            }

        }

void Parser::parse() {
   for(size_t i = 0; i < _tokens.size(); i++)
   {
        std::string token = _tokens[i];
        if(token == "server")
        {
            if(state != GLOBAL)
                throw std::runtime_error("server block found inside another block");
            if(i + 1 >= _tokens.size() || _tokens[i + 1] != "{")
                throw std::runtime_error("server must be followed by '{'");
            state = SERVER;
            server_block new_server;
            servers.push_back(new_server);
            current_server = &(servers.back());
            i++;
            continue;
        }
        if(token == "location")
        {
            if(state != SERVER)
                throw std::runtime_error("location block needs to be inside server block");
            if(i + 2 >= _tokens.size() || _tokens[i + 2] != "{")
                throw std::runtime_error("location must be followed by '{'");
            location_block new_location;
            new_location.path = _tokens[i + 1];
            current_server->locations.push_back(new_location);
            current_location = &(current_server->locations.back());
            state = LOCATION;
            i += 2;
            continue;
        }
        if(token == "}")
        {
            if(state == GLOBAL)
                throw std::runtime_error("unexpected '}' in global scope");
            if(state == LOCATION)
            {
                state = SERVER;
            }
            else if(state == SERVER)
            {
                state = GLOBAL;
                current_server = NULL;
            }
            continue;
        }
        if(token == ";")
            continue;
        if(state == SERVER)
            parse_server_block(i);
        else if(state == LOCATION)
           parse_location_block(i);
        else
            throw std::runtime_error("unknown token");
   }
   if(state != GLOBAL)
        throw std::runtime_error("missing '}'");
}

void print_servers(const std::vector<server_block>& servers) {
    
    for (size_t i = 0; i < servers.size(); ++i) {
        const server_block& s = servers[i];

        std::cout << "SERVER [" << i << "]" << std::endl;
        std::cout << "  Port: " << s.port << std::endl;
        std::cout << "  Host: " << s.host << std::endl;
        std::cout << "  Root: " << s.root << std::endl;
        std::cout << "  Index: " << s.index << std::endl;
        std::cout << "  Client Max Body Size: " << s.client_max_body_size << std::endl;
        std::cout << "  Error Pages: " << std::endl;
        if (s.error_pages.empty()) {
            std::cout << "    (None)" << std::endl;
        } else {
            for (std::map<int, std::string>::const_iterator it = s.error_pages.begin(); it != s.error_pages.end(); ++it) {
                std::cout << "    Code " << it->first << " -> " << it->second << std::endl;
            }
        }
        std::cout << "  Locations (" << s.locations.size() << "):" << std::endl;
        for (size_t j = 0; j < s.locations.size(); ++j) {
            const location_block& loc = s.locations[j];
            std::cout << "    [" << j << "] Path: " << loc.path << std::endl;
            std::cout << "      Root: " << loc.root << std::endl;
            std::cout << "      Index: " << loc.index << std::endl;
            std::cout << "      Autoindex: " << (loc.autoindex ? "ON" : "OFF") << std::endl;
            std::cout << "      Allowed Methods: ";
            if (loc.GET) std::cout << "GET ";
            if (loc.POST) std::cout << "POST ";
            if (loc.DELETE) std::cout << "DELETE ";
            std::cout << std::endl;
            if (!loc.upload_pass.empty())
                std::cout << "      Upload Pass: " << loc.upload_pass << std::endl;

            if (!loc.cgi_pass.empty()) {
                std::cout << "      CGI Pass: " << std::endl;
                for (std::map<std::string, std::string>::const_iterator it = loc.cgi_pass.begin(); it != loc.cgi_pass.end(); ++it) {
                    std::cout << "        " << it->first << " -> " << it->second << std::endl;
                }
            }
            std::cout << "    --------------------------------" << std::endl;
        }
        std::cout << "========================================" << std::endl;
    }
}