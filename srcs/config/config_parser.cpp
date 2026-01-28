#include "../../includes/config/config_parser.hpp"
#include <climits>
#include <cstddef>
#include <stdexcept>

std::string trim(std::string s)
{
    size_t size = s.size();
    for(int i = 0; i < size ; i++)
    {
        if(s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\v' && s[i] != '\f' && s[i] != '\r')
        {
            if(i == 0)
                break;
            s = s.substr(i, s.size() - 1);
            break;
        }
    }
    for(int i = size - 1; i >= 0; i--)
    {
        if(s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\v' && s[i] != '\f' && s[i] != '\r')
        {
            if(i == size - 1)
                break;
            s = s.substr(0, i + 1);
            break;
        }
    }
    return s;

}

 void config_parser::tokenize(std::string s)
 {
    std::string current_token;
    for(size_t i = 0; i < s.size(); i++)
    {
        char c = s[i];
        if(std::isspace(c))
        {
            if(!current_token.empty())
            {
                tokens.push_back(current_token);
                current_token.clear();
            }
        }
        else if(c == '{' || c == '}' || c == ';')
        {
            if(!current_token.empty())
            {
                tokens.push_back(current_token);
                current_token.clear();
            }
            tokens.push_back(std::string(1, c));
        }
        else
        {
            current_token += c;
        }
    }
 }

void config_parser::parse(const char* file_path)
{
    std::ifstream file(file_path);
    
    if(!file.is_open())
    {
        throw std::runtime_error("could not open file");
    }
    std::string s;
    while(std::getline(file, s))
    {
        s = trim(s);
        if(s.empty())
            continue;
        size_t comment_pos = s.find('#');
        if (comment_pos != std::string::npos)
            s = s.substr(0, comment_pos);
        if(s.empty())
            continue;
        //std::cout<<s<<std::endl;
        tokenize(s);
    }
    if(tokens.empty())
        throw std::runtime_error("config file is empty");
    for(int i = 0; i < tokens.size(); i++)
        std::cout<<"token number "<<i<<":  =>"<<tokens[i]<<std::endl;

}

config_parser::config_parser(const char* s)
{
   parse(s);

} 
