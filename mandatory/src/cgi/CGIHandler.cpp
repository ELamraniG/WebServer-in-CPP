#include "../../include/cgi/CGIHandler.hpp"

CGIHandler::CGIHandler() {}

// CGIHandler::CGIHandler(std::string &path, std::string &method, std::string &queryString, std::string &body, std::map<std::string, std::string> headers)
// {

// }

CGIHandler::CGIHandler(const CGIHandler &obj)
{
	(void)obj;
}

CGIHandler& CGIHandler::operator=(const CGIHandler &obj) 
{
	(void)obj;
	return (*this);
}

CGIHandler::~CGIHandler() {}
