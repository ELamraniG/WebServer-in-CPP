#include "../includes/config/Tokenizer.hpp"
#include "../includes/config/Parser.hpp"
#include "../includes/http/Request.hpp"
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
        std::vector<server_block> servers = parser.getServers();
        print_servers(servers);
        Request req1("GET", "/images/logo.png");
        Request req2("GET", "/about.html");

    }
    catch(const std::exception& e)
    {
        std::cerr<< "Error:"<<e.what()<<std::endl;
    }
    return 0;
}



