#include "../includes/config/Tokenizer.hpp"
#include "../includes/config/Parser.hpp"
#include <exception>

int main(int c, char **argv)
{
    if (c != 2)
    {
        std::cerr<<"Error: no config file present"<<std::endl;
        return 1;
    }
    try{
        
        Tokenizer tokenizer(argv[1]);
        std::vector<std::string> tokens = tokenizer.tokens;
        Parser parser(tokens);
        parser.parse();

    }
    catch(const std::exception& e)
    {
        std::cerr<< "Error:"<<e.what()<<std::endl;
    }
    return 0;
}



