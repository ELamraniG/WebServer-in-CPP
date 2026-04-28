#include "../../include/config/Parser.hpp"
#include <exception>
#include <stdexcept>

Parser::Parser(std::vector<std::string>& tokens)
    : _tokens(tokens), state(GLOBAL), current_server(NULL), current_location(NULL) {}

Parser::~Parser() {}

std::vector<Server_block> Parser::getServers() const 
{
    return servers;
}

static void mark_seen(std::set<std::string>& seen, const std::string& key)
{
    if (seen.count(key))
        throw std::runtime_error("duplicate directive: '" + key + "'");
    seen.insert(key);
}

static bool split_listen(const std::string& val, std::string& host_out, std::string& port_out)
{
    size_t colon = val.find(':');
    if (colon == std::string::npos) 
    {
        host_out = "";
        port_out = val;
    } 
    else 
    {
        host_out = val.substr(0, colon);
        port_out = val.substr(colon + 1);
        if (host_out.empty())
            return false;
    }
    return isvalidport(port_out);
}

void Parser::parse_server_block(size_t &i)
{
    std::string key = _tokens[i];

    if (key == "listen")
    {
        if (i + 2 >= _tokens.size() || _tokens[i + 2] != ";")
            throw std::runtime_error("missing semicolon after 'listen'");
        mark_seen(_server_seen, "listen");
        std::string host_part, port_part;
        if (!split_listen(_tokens[i + 1], host_part, port_part))
            throw std::runtime_error("invalid listen value: " + _tokens[i + 1]);
        current_server->port = port_part;
        if (!host_part.empty())
            current_server->host = host_part;
        i += 2;
    }
    else if (key == "error_page")
    {
        if (i + 3 >= _tokens.size() || _tokens[i + 3] != ";")
            throw std::runtime_error("missing semicolon after 'error_page'");
        int error = isvalid_error_number(_tokens[i + 1]);
        if (error == -1)
            throw std::runtime_error("invalid error page number: " + _tokens[i + 1]);
        if (current_server->error_pages.count(error))
            throw std::runtime_error("duplicate error_page for code");
        current_server->error_pages[error] = _tokens[i + 2];
        i += 3;
    }
    else if (key == "client_max_body_size")
    {
        if (i + 2 >= _tokens.size() || _tokens[i + 2] != ";")
            throw std::runtime_error("missing semicolon after 'client_max_body_size'");
        mark_seen(_server_seen, "client_max_body_size");
        current_server->client_max_body_size = isvalid_client_number(_tokens[i + 1]);
        i += 2;
    }
    else if (key == "root")
    {
        if (i + 2 >= _tokens.size() || _tokens[i + 2] != ";")
            throw std::runtime_error("missing semicolon after 'root'");
        mark_seen(_server_seen, "root");
        current_server->root = _tokens[i + 1];
        i += 2;
    }
    else if (key == "index")
    {
        if (i + 2 >= _tokens.size() || _tokens[i + 2] != ";")
            throw std::runtime_error("missing semicolon after 'index'");
        mark_seen(_server_seen, "index");
        current_server->index = _tokens[i + 1];
        i += 2;
    }
    else
    {
        throw std::runtime_error("unknown directive in server: '" + key + "'");
    }
}

void Parser::parse_location_block(size_t &i)
{
    std::string key = _tokens[i];

    if (key == "root")
    {
        if (i + 2 >= _tokens.size() || _tokens[i + 2] != ";")
            throw std::runtime_error("root in location: missing value or semicolon");
        mark_seen(_location_seen, "root");
        current_location->root = _tokens[i + 1];
        i += 2;
    }
    else if (key == "index")
    {
        if (i + 2 >= _tokens.size() || _tokens[i + 2] != ";")
            throw std::runtime_error("index in location: missing value or semicolon");
        mark_seen(_location_seen, "index");
        current_location->index = _tokens[i + 1];
        i += 2;
    }
    else if (key == "autoindex")
    {
        if (i + 2 >= _tokens.size() || _tokens[i + 2] != ";")
            throw std::runtime_error("autoindex: missing value or semicolon");
        mark_seen(_location_seen, "autoindex");
        if (_tokens[i + 1] == "on")
            current_location->autoindex = true;
        else if (_tokens[i + 1] == "off")
            current_location->autoindex = false;
        else
            throw std::runtime_error("autoindex must be 'on' or 'off'");
        i += 2;
    }
    else if (key == "allowed_methods")
    {
        mark_seen(_location_seen, "allowed_methods");
        bool any = false;
        while (i + 1 < _tokens.size() && _tokens[i + 1] != ";")
        {
            const std::string& m = _tokens[i + 1];
            if (m == "GET")
                current_location->methods["GET"] = true;
            else if (m == "POST")
                current_location->methods["POST"] = true;
            else if (m == "DELETE")
                current_location->methods["DELETE"] = true;
            else
                throw std::runtime_error("unknown method: " + m);
            any = true;
            i++;
        }
        if (!any)
            throw std::runtime_error("allowed_methods needs at least one method");
        if (i + 1 >= _tokens.size() || _tokens[i + 1] != ";")
            throw std::runtime_error("allowed_methods: missing semicolon");
        i += 1;
    }
    else if (key == "upload_pass")
    {
        if (i + 2 >= _tokens.size() || _tokens[i + 2] != ";")
            throw std::runtime_error("upload_pass: missing value or semicolon");
        mark_seen(_location_seen, "upload_pass");
        current_location->upload_pass = _tokens[i + 1];
        i += 2;
    }
    else if (key == "cgi_pass")
    {
        if (i + 3 >= _tokens.size() || _tokens[i + 3] != ";")
            throw std::runtime_error("cgi_pass: missing value or semicolon");
        std::string ext = _tokens[i + 1];
        if (ext.empty() || ext[0] != '.')
            throw std::runtime_error("CGI extension must start with '.'");
        if (current_location->cgi_pass.count(ext))
            throw std::runtime_error("duplicate cgi_pass for extension: " + ext);
        current_location->cgi_pass[ext] = _tokens[i + 2];
        i += 3;
    }
    else if (key == "return")
    {
        if (i + 3 >= _tokens.size() || _tokens[i + 3] != ";")
            throw std::runtime_error("missing semicolon after 'return'");
        if (current_location->redirect.size() >= 1)
            throw std::runtime_error("only one return directive allowed per location");
        int return_code = isvalid_return_number(_tokens[i + 1]);
        if (return_code == -1)
            throw std::runtime_error("invalid redirection code: " + _tokens[i + 1]);
        current_location->redirect[return_code] = _tokens[i + 2];
        i += 3;
    }
    else
    {
        throw std::runtime_error("unknown directive in location: '" + key + "'");
    }
}

void Parser::parse()
{
    for (size_t i = 0; i < _tokens.size(); i++)
    {
        std::string token = _tokens[i];

        if (token == "server")
        {
            if (state != GLOBAL)
                throw std::runtime_error("server block found inside another block");
            if (i + 1 >= _tokens.size() || _tokens[i + 1] != "{")
                throw std::runtime_error("server must be followed by '{'");
            state = SERVER;
            Server_block new_server;
            servers.push_back(new_server);
            current_server = &(servers.back());
            _server_seen.clear();
            i++;
            continue;
        }
        if (token == "location")
        {
            if (state != SERVER)
                throw std::runtime_error("location block must be inside a server block");
            if (i + 2 >= _tokens.size() || _tokens[i + 2] != "{")
                throw std::runtime_error("location must be followed by path and '{'");
            std::string path = _tokens[i + 1];
            if (path.empty() || path[0] != '/')
                throw std::runtime_error("location path must start with '/': " + path);
            for (size_t k = 0; k < current_server->locations.size(); ++k) 
            {
                if (current_server->locations[k].path == path)
                    throw std::runtime_error("duplicate location path: " + path);
            }

            location_block new_location;
            new_location.path = path;
            current_server->locations.push_back(new_location);
            current_location = &(current_server->locations.back());
            _location_seen.clear();
            state = LOCATION;
            i += 2;
            continue;
        }
        if (token == "}")
        {
            if (state == GLOBAL)
                throw std::runtime_error("unexpected '}' in global scope");
            if (state == LOCATION) 
            {
                state = SERVER;
                current_location = NULL;
            } else if (state == SERVER) 
            {
                if (current_server->port.empty())
                    throw std::runtime_error("server block is missing 'listen'");
                state = GLOBAL;
                current_server = NULL;
            }
            continue;
        }
        if (token == ";")
            continue;

        if (state == SERVER)
            parse_server_block(i);
        else if (state == LOCATION)
            parse_location_block(i);
        else
            throw std::runtime_error("unexpected token at top level: '" + token + "'");
    }

    if (state != GLOBAL)
        throw std::runtime_error("missing '}'");
    if (servers.empty())
        throw std::runtime_error("no server block defined");
    for (size_t a = 0; a < servers.size(); ++a) 
    {
        for (size_t b = a + 1; b < servers.size(); ++b) 
        {
            if (servers[a].host == servers[b].host &&
                servers[a].port == servers[b].port)
                throw std::runtime_error(
                    "duplicate server bind: " + servers[a].host +
                    ":" + servers[a].port);
        }
    }
}
