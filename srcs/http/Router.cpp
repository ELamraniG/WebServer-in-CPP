#include "../../includes/http/Router.hpp"
#include <iostream>
#include <string>


     server_block& Router::match_server(Request& req, std::vector<server_block>& servers)
    {
        std::string host;
        //depending on the browser the request hos can be Host or host
        if(req.headers.count("Host"))
           host = req.headers.at("Host");
        else if (req.headers.count("host"))
            host = req.headers.at("host");
        else
        {
            return (servers[0]);
        }
        std::string hostname = host;
        std::string _port = "80";
        size_t clon_pos = host.find(':');
        if(clon_pos != std::string::npos)
        {
            _port = host.substr(clon_pos + 1);
            hostname = host.substr(0, clon_pos);

        }
        server_block *default_server = NULL;
        for(size_t i = 0; i < servers.size(); i++)
        {
            if(servers[i].port == _port)
            {
                if(default_server != NULL)
                    default_server = &servers[i];
                if(hostname == servers[i].host)
                    return (servers[i]);
            }
            
        }
        return (servers[0]);
    }
bool   find_longest_match(std::string s2, std::string s1)
{
    if(s1.length() > s2.length())
        return false;
    return s2.compare(0, s1.length(), s1) == 0;
}

// request uri	location path	match	reason
// /images/img/hh	/images	    YES	    request is inside location
// /images	        /images	    YES	    exact match
// images	      /images/img/hh NO	    request is outside parent of location.
location_block *Router::match_location(Request& req, server_block& server)
{
    location_block *best_loc = NULL;
    size_t best_len = 0;
    for(size_t i = 0; i < server.locations.size(); i++)
    {
        location_block *loc = &(server.locations[i]);
        if(find_longest_match(req.uri, loc->path))
        {
            if(req.uri.length() >  loc->path.length())
            {
                //make sure we didnt find a match like this location path : /img_hh/gg and req_path : /img
                if(req.uri[loc->path.length()] != '/' && loc->path != "/")
                {
                    continue;
                }
            }

            if(loc->path.length() >  best_len)
            {
                best_len = loc->path.length();
                best_loc = loc;
            }
        }
    }
    return best_loc;
}
std::string Router::get_path(Request& req, server_block& server, location_block* location)
{
    if (!location)
        std::cout<<"location is NULL"<<std::endl;

    std::string root = location->root;
    std::string req_path = req.uri;
    if(root.empty())
    {
        root = server.root;
    }
    if(root[root.size() - 1] == '/' && req_path[0] == '/')
        root+= req_path.substr(1);
    else
    {
        root += req_path;
    }
    return root;

}

