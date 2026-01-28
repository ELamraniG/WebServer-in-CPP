#include "../includes/config/config_parser.hpp"
#include <exception>

int main(int c, char **argv)
{
    if (c != 2)
    {
        std::cerr<<"Error: no config file present"<<std::endl;
        return 1;
    }
    try{
        
        config_parser y(argv[1]);
    }
    catch(const std::exception& e)
    {
        std::cerr<< "Error:"<<e.what()<<std::endl;
    }
    return 0;
}



