#include "../../include/http/ResponseBuilder.hpp"
#include <cstddef>
#include <ctime>
#include <sstream>

ResponseBuilder::ResponseBuilder()
{
}

std::string ResponseBuilder::reasonPhrase(unsigned int code)
{
	switch (code)
	{
	// 2xx
	case 200:
		return ("OK");
	case 201:
		return ("Created");
	case 204:
		return ("No Content");
	// 3xx
	case 301:
		return ("Moved Permanently");
	case 302:
		return ("Found");
	case 307:
		return ("Temporary Redirect");
	case 308:
		return ("Permanent Redirect");
	// 4xx
	case 400:
		return ("Bad Request");
	case 403:
		return ("Forbidden");
	case 404:
		return ("Not Found");
	case 405:
		return ("Method Not Allowed");
	case 408:
		return ("Request Timeout");
	case 413:
		return ("Payload Too Large");
	case 414:
		return ("URI Too Long");
	// 5xx
	case 500:
		return ("Internal Server Error");
	case 501:
		return ("Not Implemented");
	case 502:
		return ("Bad Gateway");
	case 504:
		return ("Gateway Timeout");
	case 505:
		return ("HTTP Version Not Supported");
	default:
	{
		if (code >= 200 && code < 300)
			return ("OK");
		if (code >= 300 && code < 400)
			return ("Redirection");
		if (code >= 400 && code < 500)
			return ("Client Error");
		return ("Server Error");
	}
	}
}

static std::string httpDate()
{
	char	buf[64];

	std::time_t now = std::time(NULL);
	struct std::tm *gmt = std::gmtime(&now);
	// Sun, 06 Nov 1994 08:49:37 GMT
	std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
	return (std::string(buf));
}

std::string ResponseBuilder::build(const Response &resp) const
{
	std::ostringstream out;

	out << "HTTP/1.0 " << resp.statusCode << " " << reasonPhrase(resp.statusCode) << "\r\n";

	out << "Date: " << httpDate() << "\r\n";

	out << "Server: webserv/1.0\r\n";

	for (std::map<std::string,
		std::string>::const_iterator it = resp.headers.begin(); it != resp.headers.end(); ++it)
	{
		std::string key = it->first;

		std::string lower;
		for (std::string::size_type i = 0; i < key.size(); ++i)
			lower
				+= static_cast<char>(std::tolower(static_cast<unsigned char>(key[i])));
		if (lower == "content-type" || lower == "content-length")
			continue ;
		out << it->first << ": " << it->second << "\r\n";
	}

	if (resp.statusCode != 204 && resp.statusCode != 304)
	{
		if (!resp.contentType.empty())
			out << "Content-Type: " << resp.contentType << "\r\n";

		out << "Content-Length: " << resp.body.size() << "\r\n";
	}

	out << "Connection: close\r\n";

	out << "\r\n";

	if (resp.statusCode != 204 && resp.statusCode != 304)
		out << resp.body;

	return (out.str());
}

std::string ResponseBuilder::errorBody(int code)
{
    std::string r = reasonPhrase(code);
    std::ostringstream b;
    b << "<!DOCTYPE html><html><head><meta charset='utf-8'>"
      << "<title>" << code << "</title>"
      << "<style>"
      << "*{margin:0;padding:0;box-sizing:border-box}"
      << "body{background:#0a0a0a;color:#888;font-family:monospace;"
      << "display:flex;align-items:center;justify-content:center;height:100vh}"
      << "main{text-align:center;border:1px solid #1e1e1e;padding:48px 64px}"
      << "h1{font-size:72px;font-weight:400;color:#fff;letter-spacing:-2px}"
      << "p{margin-top:12px;font-size:13px;letter-spacing:2px;text-transform:uppercase}"
      << "</style></head>"
      << "<body><main><h1>" << code << "</h1><p>" << r << "</p></main></body></html>";
    return b.str();
}

std::string ResponseBuilder::buildError(int code) const
{
    Response resp;
    resp.statusCode = code;
    resp.contentType = "text/html";
    resp.body = errorBody(code);
    return build(resp);
}

std::string ResponseBuilder::buildCgiResponse(std::string cgiResult, RouteConfig &Route) const
{
	std::ostringstream out;

	out << "HTTP/1.0 " << 200 << " " << reasonPhrase(200) << "\r\n";
	out << "Date: " << httpDate() << "\r\n";
	out << "Server: webserv/1.0\r\n";
	size_t sep = cgiResult.find("\r\n\r\n");
	if (sep == std::string::npos)
  {
    Response r = MethodHandler::makeError(500, Route);
    std::string res = build(r);
		return (res);
  }
	std::string headersPart = cgiResult.substr(0, sep);
	std::string bodyPart = cgiResult.substr(sep + 4);

	size_t i = 0;
	size_t pos = 0;
	while (i < headersPart.size())
	{
		pos = headersPart.find("\r\n", i);
		if (pos == std::string::npos)
		{
			std::string headersLine = headersPart.substr(i);
			out << headersLine << "\r\n";
			break ;
		}
		pos += 2;
		std::string headersLine = headersPart.substr(i, pos - i);
		out << headersLine;
		i = pos;
	}
	out << "\r\n";
	out << bodyPart;
	return (out.str());
}