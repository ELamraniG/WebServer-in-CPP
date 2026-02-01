#include "../includes/config/Tokenizer.hpp"
#include "../includes/config/Parser.hpp"
#include "../includes/http/Router.hpp"
#include "../includes/http/Request.hpp"
#include "../includes/http/Respond.hpp"
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
        Request req1("GET", "/index.html");
        Request req2("GET", "/");
        server_block s1 = Router::match_server(req1, servers);
        location_block* loc = Router::match_location(req1, s1);
        std::string error_path = Router::get_error_path(s1);
        std::string path = Router::get_path(req1, s1, loc);
        std::string index = loc->index;
        if(index.empty())
            index = s1.index;
        Respond res;
        res = res.generate_response(path, error_path, loc->autoindex, index, req1.uri, loc->methods,req1.method, loc->redirect);
       std::cout<<res.to_string();
    }
    catch(const std::exception& e)
    {
        std::cerr<< "Error:"<<e.what()<<std::endl;
    }
    return 0;
}


//FOR TESTING!!!!!!!

// #include <iostream>
// #include <string>
// #include <vector>
// #include <iomanip>



// // --- COLOR MACROS FOR OUTPUT ---
// #define RESET   "\033[0m"
// #define GREEN   "\033[32m"
// #define RED     "\033[31m"
// #define CYAN    "\033[36m"

// // --- HELPER TO PRINT RESULTS ---
// void assertMatch(std::string testName, 
//                  std::string expectedPath, 
//                  std::string actualPath, 
//                  bool expectNull = false, 
//                  void* resultPtr = NULL) 
// {
//     std::cout << std::left << std::setw(50) << testName;
    
//     if (expectNull) {
//         if (resultPtr == NULL) std::cout << GREEN << "[PASS]" << RESET << std::endl;
//         else std::cout << RED << "[FAIL] (Expected NULL, got ptr)" << RESET << std::endl;
//         return;
//     }

//     if (resultPtr == NULL) {
//         std::cout << RED << "[FAIL] (Got NULL)" << RESET << std::endl;
//         return;
//     }

//     if (expectedPath == actualPath) {
//         std::cout << GREEN << "[PASS]" << RESET << std::endl;
//     } else {
//         std::cout << RED << "[FAIL]" << RESET << std::endl;
//         std::cout << "      Expected: " << expectedPath << std::endl;
//         std::cout << "      Actual:   " << actualPath << std::endl;
//     }
// }

// int main(int argc, char **argv) {
//     (void)argc;
//     (void)argv;

//     std::cout << CYAN << "--- LOADING CONFIG (router_test.conf) ---" << RESET << std::endl;
    
//     try {
//         Tokenizer tokenizer(argv[1]);
//                  std::vector<std::string> tokens = tokenizer.tokens;

//         Parser parser(tokens);
//         parser.parse();
//         std::vector<server_block> servers = parser.getServers();

//         std::cout << "Loaded " << servers.size() << " servers.\n" << std::endl;
//          print_servers(servers);

//         // ==========================================
//         // TEST GROUP 1: SERVER SELECTION
//         // ==========================================
//         std::cout << CYAN << "--- TEST GROUP 1: SERVER SELECTION ---" << RESET << std::endl;

//         // Test 1.1: Default Server (No Host header)
//         Request req1("GET", "/");
//         req1.headers["Host"] = "localhost:8080"; 
//         server_block s1 = Router::match_server(req1, servers);
//         // We expect the first server defined for 8080 (Server 1)
//         assertMatch("1. Default Server Selection", "/var/www/default", s1.root, false, &s1);

//         // Test 1.2: Specific Hostname (test.com)
//         Request req2("GET", "/");
//         req2.headers["Host"] = "test.com:8080";
//         server_block s2 = Router::match_server(req2, servers);
//         // We expect Server 2
//         assertMatch("2. Hostname Match (test.com)", "/var/www/test_com", s2.root, false, &s2);

//         // Test 1.3: Different Port (9090)
//         Request req3("GET", "/");
//         req3.headers["Host"] = "localhost:9090";
//         server_block s3 = Router::match_server(req3, servers);
//         // We expect Server 3
//         assertMatch("3. Port Match (9090)", "/var/www/api", s3.root, false, &s3);


//         // ==========================================
//         // TEST GROUP 2: LOCATION MATCHING
//         // ==========================================
//         std::cout << "\n" << CYAN << "--- TEST GROUP 2: LOCATION MATCHING ---" << RESET << std::endl;
        
//         // Use Server 1 for these tests
//         server_block& srv = servers[0]; 

//         // Test 2.1: Simple Root Match
//         Request req4("GET", "/index.html");
//         location_block* loc4 = Router::match_location(req4, srv);
//         assertMatch("4. Root Location /", "/", loc4->path, false, loc4);

//         // Test 2.2: Exact Match Subfolder
//         Request req5("GET", "/images/pic.jpg");
//         location_block* loc5 = Router::match_location(req5, srv);
//         assertMatch("5. Prefix Match /images", "/images", loc5->path, false, loc5);

//         // Test 2.3: Longest Prefix Match
//         Request req6("GET", "/images/png/icon.png");
//         location_block* loc6 = Router::match_location(req6, srv);
//         assertMatch("6. Longest Prefix /images/png", "/images/png", loc6->path, false, loc6);

//         // ==========================================
//         // TEST GROUP 3: PATH CONSTRUCTION & EDGE CASES
//         // ==========================================
//         std::cout << "\n" << CYAN << "--- TEST GROUP 3: PHYSICAL PATHS & BUGS ---" << RESET << std::endl;

//         // Test 3.1: Partial Word Match (THE BUG TEST)
//         // Request "/images_backup" should NOT match "/images"
//         Request req7("GET", "/images_backup/file.txt");
//         location_block* loc7 = Router::match_location(req7, srv);
        
//         // If it matches "/", path is "/"
//         // If it incorrectly matches "/images", path is "/images"
//         // We expect it to fallback to "/" because "/images_backup" != "/images"
//         std::string expected = "/"; 
//         assertMatch("7. Partial Word Safety (images_backup)", expected, loc7->path, false, loc7);

//         // Test 3.2: Path Construction (Root + URI)
//         // Location /images (root /var/www/default/photos)
//         // URI /images/pic.jpg
//         // Result should be: /var/www/default/photos/images/pic.jpg
//         std::string realPath = Router::get_path(req5, srv, loc5);
//         assertMatch("8. Physical Path Construction", "/var/www/default/photos/images/pic.jpg", realPath, false, &realPath);

//         // Test 3.3: Server Root Fallback (Server 3)
//         // Server 3 has location /api with NO root. It should inherit server root.
//         server_block& srv3 = servers[2];
//         Request req8("GET", "/api/data.json");
//         location_block* loc8 = Router::match_location(req8, srv3);
//         std::string inheritedPath = Router::get_path(req8, srv3, loc8);
//         assertMatch("9. Root Inheritance", "/var/www/api/api/data.json", inheritedPath, false, loc8);

//     } catch (std::exception& e) {
//         std::cout << RED << "CRASH: " << e.what() << RESET << std::endl;
//     }

//     return 0;
// }

