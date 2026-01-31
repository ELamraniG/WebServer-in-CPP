#include "../../includes/http/Respond.hpp"
#include <dirent.h>
Respond::Respond():_status(0), _body("")
{
    _headers[""] = "";
}
void Respond::set_status(int status)
{
    _status = status;
}
void Respond::set_header(const std::string& key, const std::string& value)
{
    _headers[key] = value;
}
void Respond::set_body(const std::string& body)
{
    _body = body;
}

bool readfile(std::string& path, std::string& content)
{
    std::ifstream file(path.c_str());
    if (!file.is_open())
        return false;
    std::stringstream buffer;
    buffer<< file.rdbuf();
    
    content = buffer.str();
    return true;

}
std::string set_mimetype(std::string& path)
{
    size_t pos = path.find_last_of('.');
    if(pos == std::string::npos)
        return "text/plain";
    std::string mime = path.substr(pos);
    if(mime == ".html" || mime == ".htm")
        return "text/html";
    else if(mime == ".css")
        return "text/css";
    else if(mime == ".js")
        return "application/javascript";
    else if(mime == ".png")
        return "image/png";
    else if(mime == ".jpg" || mime == ".jpeg")
        return "image/jpeg";
    else if(mime == ".gif")
        return "image/gif";
    else if(mime == ".txt")
        return "text/plain";
    else if(mime == ".ico")
        return "image/x-icon";
    else if(mime == ".svg")
        return "image/svg+xml";
    else
        return "application/octet-stream";
}
bool is_directory(std::string& path)
{
    struct stat statbuf;
    //check if file exist
    if (stat(path.c_str(), &statbuf) != 0)
        return false;
    //check if the bit of dir is set to 1 . (it is a dir)
    return S_ISDIR(statbuf.st_mode);
}
std::string generate_auto_index(std::string path, std::string uri)
{
    //need to study more about opendi and dirent
    DIR* dir = opendir(path.c_str());
    if(dir == NULL) // needs to double check how should i handle no permession to open a dir
        return "";
    std::string html = "<html><head><h1>Index of " + uri + "</h1></head><body>";
    struct dirent* entry;
    while((entry = readdir(dir)) != NULL)
    {
        //get the file name
        std::string name = entry->d_name;
        //generate a clickable link for each file
        html +=  "<li><a href= \"" + uri + (uri[uri.size() - 1] == '/'? "" : "/") + name + "\">" + name + "</a></li>";
    }
    html += "</ul></body> </html>";
    closedir(dir);
    return html;

}
Respond Respond::generate_response(std::string &path, std::string &error_path, bool autoindex, std::string index, std::string uri)
{
    std::string content;
    Respond resp;
    if(is_directory(path))
    {
        
        std::string index_path = path + (path[path.size() - 1] =='/' ? "": "/") + index;
        if(readfile(index_path, content))
        {
            resp.set_status(200);
            resp.set_body(content);
            resp.set_header("Content-Type", set_mimetype(path));
            return resp;
        }
        if(autoindex)
        {
            content = generate_auto_index(path, uri);
            resp.set_status(200);
            resp.set_body(content);
            resp.set_header("Content-Type", set_mimetype(path));
            return resp;

            
        }
        resp.set_status(403);
        resp.set_body("<h1>403 forbeddin<h1/>"); //need to check if we should handle all types of error pages or its okey for some to be hardcoed
        resp.set_header("Content-Type", set_mimetype(path));
    }
    if(readfile(path, content))
    {
        resp.set_status(200);
        resp.set_body(content);
        resp.set_header("Content-Type", set_mimetype(path));
        return resp;
    }
    else
    {
         if(readfile(error_path, content))
         {
            resp.set_status(404);
            resp.set_body(content);
            resp.set_header("Content-Type", "text/html");
            return resp;
         }
         else
         {
            resp.set_body("<h1>404</h1><p>no file found. even the 404</p>");
            return resp;
         }
        
    }
    return resp;
}