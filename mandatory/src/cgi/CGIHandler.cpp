#include "../../include/cgi/CGIHandler.hpp"
#include <cstring>

CGIHandler::CGIHandler() {}

CGIHandler::CGIHandler(std::string &path, std::string &method, std::string &queryString, std::string &body, std::map<std::string, std::string> headers) :
	_scriptPath(path),
	_method(method),
	_queryString(queryString),
	_body(body),
	_headers(headers)
{
	buildEnv();
}

CGIHandler::CGIHandler(const CGIHandler &obj)
{
	(void)obj;
}

CGIHandler& CGIHandler::operator=(const CGIHandler &obj) 
{
	(void)obj;
	return (*this);
}

bool CGIHandler::isCGIRequest() const
{
	return (true);
}

std::string getPath()
{
	std::string	path;
	size_t		pos;

	path = "/usr/local/bin:/usr/bin:/bin";
	for (int i=0; environ[i]; i++)
	{
		if (!strncmp(environ[i], "PATH=", 5))
			path = environ[i];
	}
	pos = path.find("/usr/");
	if (pos != std::string::npos)
		path = path.substr(pos);
	return (path);
}

void CGIHandler::buildEnv()
{
	_envStrings.push_back("REQUEST_METHOD=" + _method);
	_envStrings.push_back("QUERY_STRING=" + _queryString);
	_envStrings.push_back("SCRIPT_FILENAME=" + _scriptPath);
	_envStrings.push_back("SCRIPT_NAME=www/" + _scriptPath);
	_envStrings.push_back("SERVER_PROTOCOL=HTTP/1.0");
	_envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_envStrings.push_back("PATH=" + getPath());
	if (_method == "POST")
	{
		_envStrings.push_back("CONTENT_TYPE=" + _headers["content-type"]);
		_envStrings.push_back("CONTENT_LENGTH=" + _headers["content-length"]);
	}

	// request method
	// query string
	// script filename
	// script name
	// server protocol
	// gateway interface
	// +++ POST
	// content type
	// content length
}

CGIHandler::~CGIHandler() {}
