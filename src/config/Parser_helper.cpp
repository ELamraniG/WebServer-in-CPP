#include "../../include/config/Parser.hpp"

bool isvalidport(std::string& port)
{
    std::stringstream s(port);
    int _port;
    if (!(s >> _port) || !s.eof())
        return false;
    if (_port < 1 || _port > 65535)
        return false;
    return true;
}

int isvalid_error_number(std::string& error_number)
{
    std::stringstream s(error_number);
    int _error_number;
    if (!(s >> _error_number) || !s.eof())
        return -1;
    if (_error_number < 300 || _error_number > 599)
        return -1;
    return _error_number;
}

int isvalid_return_number(std::string& return_number)
{
    std::stringstream s(return_number);
    int _return_number;
    if (!(s >> _return_number) || !s.eof())
        return -1;
    if (_return_number < 300 || _return_number > 399)
        return -1;
    return _return_number;
}

unsigned long isvalid_client_number(std::string& client_number)
{
    if (client_number.empty())
        throw std::runtime_error("empty client_max_body_size value");

    unsigned long mult = 1;
    std::string num_str = client_number;
    char last = num_str[num_str.size() - 1];

    if (last == 'm' || last == 'M') {
        mult = 1024UL * 1024UL;
        num_str = num_str.substr(0, num_str.size() - 1);
    } else if (last == 'k' || last == 'K') {
        mult = 1024UL;
        num_str = num_str.substr(0, num_str.size() - 1);
    } else if (last == 'g' || last == 'G') {
        mult = 1024UL * 1024UL * 1024UL;
        num_str = num_str.substr(0, num_str.size() - 1);
    }
    if (num_str.empty())
        throw std::runtime_error("client_max_body_size: missing number");

    std::stringstream s(num_str);
    unsigned long value;
    if (!(s >> value) || !s.eof())
        throw std::runtime_error("client_max_body_size: not a number");

    return value * mult;
}
