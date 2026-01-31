#include "../../includes/config/Parser.hpp"

bool isvalidport( std::string& port)
{
    std::stringstream s(port);
    int _port;
    if(!(s>>_port) || !s.eof())
        return false;
    if(_port < 1 || _port > 65535)
        return false;
    return true;
}
int isvalid_error_number( std::string& error_number)
{
    std::stringstream s(error_number);
    int _error_number;
    if(!(s>>_error_number) || !s.eof())
    {
      return -1;
    }
    if(_error_number < 300 || _error_number > 599)
    {
       return -1;
    }
    return _error_number;
}
unsigned long isvalid_client_number( std::string& client_number)
{
    char identifier = client_number[client_number.size() - 1];
    std::cout<<identifier;
    unsigned long t = 1;
    if(identifier == 'm' || identifier == 'M' )
    {
        t = 1024 * 1014;
    }
    else if(identifier == 'k' || identifier == 'K' )
    {
        t = 1024;
    }
    else if(identifier == 'g' || identifier == 'G' )
    {
        t = 1024 * 1024 * 1024;
    }
    else
        throw std::runtime_error("not a valid identifier");
    client_number = client_number.substr(0, client_number.size() - 1);
    std::stringstream s(client_number);
    unsigned long _client_number;
    if(!(s>>_client_number) || !s.eof())
        return -1;
    if(_client_number < 1 || _client_number > 9)
        return -1;
    return _client_number * t;
}