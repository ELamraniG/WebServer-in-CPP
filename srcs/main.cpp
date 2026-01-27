#include "../includes/config/config_parser.hpp"

int main(int c, char **argv)
{
    if (c != 2)
    {
        std::cerr<<"Error"<<std::endl;
    }

    config_parser y(argv[1]);
}



