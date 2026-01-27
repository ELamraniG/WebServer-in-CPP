#include "../../includes/config/config_parser.hpp"

  bool has_only_space(std::string s)
  {
    unsigned int size = s.size();
    for (unsigned int i = 0 ; i< size; i++)
    {
        if(s[i] != '\t' || s[i] != '\n' || s[i] != '\v' || s[i] != '\f' || s[i] != '\r')
            return false;
    }
    return true;
  }

 bool tokenize(std::ifstream &file, std::vector<std::string> &vect)
 {
    std::string s;
    while(std::getline(file, s))
    {
        if(s.empty() || has_only_space(s) == true)
            continue;

    }
    if(vc.empty())
        return false;

 }

bool parse(const char* file_path, std::vector<std::string> &vect)
{
    std::ifstream file(file_path);
    if(!file.is_open())
    {
        std::cerr<<"Error: could not open file"<<std::endl;
        retun false;
    }
    if(tokenize(&file, &vect) == false)
        return false;

}

config_parser::config_parser(const char* s)
{
    if(parse(s, &vect) == false)
        return ;

} 
