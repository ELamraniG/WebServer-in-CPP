#include "../../include/logger/Logger.hpp"

#include <sstream>
#include <iostream>

std::string	Logger::badge(const std::string& bg, const std::string& label)
{
	return (C_BOLD() + bg + " " + label + " " + C_RESET() + " ");
}

std::string	Logger::colorOf(HttpStatus code)
{
	if (code == HTTP_CLIENT_DISCONNECTED)
		return (C_GRAY());
	if (code >= 500)
		return (C_RED());
	if (code >= 400)
		return (C_ORANGE());
	if (code >= 300)
		return (C_CYAN());
	if (code >= 200)
		return (C_GREEN());
	return (C_GRAY());
}

std::string	Logger::labelOf(HttpStatus code)
{
	switch (code)
	{
		case HTTP_CLIENT_DISCONNECTED:		return ("CLOSED");
		case HTTP_OK:						return ("200 OK");
		case HTTP_CREATED:					return ("201 CREATED");
		case HTTP_NO_CONTENT:				return ("204 NO CONTENT");
		case HTTP_MOVED_PERMANENTLY:		return ("301 MOVED");
		case HTTP_FOUND:					return ("302 FOUND");
		case HTTP_BAD_REQUEST:				return ("400 BAD REQUEST");
		case HTTP_FORBIDDEN:				return ("403 FORBIDDEN");
		case HTTP_NOT_FOUND:				return ("404 NOT FOUND");
		case HTTP_METHOD_NOT_ALLOWED:		return ("405 NOT ALLOWED");
		case HTTP_REQUEST_TIMEOUT:			return ("408 TIMEOUT");
		case HTTP_CONTENT_TOO_LARGE:		return ("413 TOO LARGE");
		case HTTP_INTERNAL_SERVER_ERROR:	return ("500 INTERNAL ERROR");
		case HTTP_NOT_IMPLEMENTED:			return ("501 NOT IMPLEMENTED");
		case HTTP_BAD_GATEWAY:				return ("502 BAD GATEWAY");
		case HTTP_GATEWAY_TIMEOUT:			return ("504 GW TIMEOUT");
		default:							return ("UNKNOWN");
	}
}

std::string	Logger::statusStr(HttpStatus code)
{
	return (C_BOLD() + colorOf(code) + "[" + labelOf(code) + "]" + C_RESET());
}

void	Logger::serverStart(const std::string& host, const std::string& port, int fd)
{
	std::ostringstream	oss;

	oss << port;
	std::cout << badge(BG_GREEN(), "SERVER")
				<< C_WHITE() + "listening on "
				<< C_BOLD() << C_GREEN() << host << ":" << oss.str() << C_RESET()
				<< C_GRAY() << "  fd[" << fd << "]" << C_RESET()
				<< std::endl;
}

void	Logger::clientConnected(int fd, const std::string& ip, std::string& port)
{
	std::ostringstream	oss;

	oss << port;
	std::cout << badge(BG_CYAN(), "CONNECT")
				<< C_CYAN() << ip << ":" << oss.str() << C_RESET()
				<< C_GRAY() << "  fd[" << fd << "]" << C_RESET()
				<< std::endl;
}

void	Logger::clientDisconnected(int fd, HttpStatus code)
{
	std::ostringstream	oss;

	oss << fd;
	std::cout << badge(BG_GRAY(), "DISCONNECT")
				<< C_GRAY() << "fd[" << fd << "]" << C_RESET()
				<< "  " << statusStr(code)
				<< std::endl;
}

void	Logger::clientTimeout(int fd)
{
	std::cout << badge(BG_YELLOW(), "TIMEOUT")
				<< C_YELLOW() << "client fd[" << fd << "] took too long" << C_RESET()
				<< "  " << statusStr(HTTP_REQUEST_TIMEOUT)
				<< std::endl;
}

void	Logger::staticFile(const std::string& method, const std::string& uri, HttpStatus code)
{
	std::cout << badge(BG_CYAN(), "STATIC")
				<< C_BOLD() << C_WHITE() << method << C_RESET()
				<< C_GRAY() << "  " << uri << C_RESET()
				<< "  " << statusStr(code)
				<< std::endl;
}

void	Logger::cgiRun(int fd, const std::string& script)
{
	std::cout << badge(BG_MAGENTA(), "CGI")
			  << C_MAGENTA() << "spawned" << C_RESET()
			  << C_GRAY() << "  fd[" << fd << "]" << C_RESET()
			  << "  " << C_WHITE() << script << C_RESET()
			  << std::endl;
}

void	Logger::cgiDone(int fd, const std::string& script, HttpStatus code)
{
	std::cout << badge(BG_MAGENTA(), "CGI")
				<< C_MAGENTA() << "done   " << C_RESET()
				<< C_GRAY() << "  fd[" << fd << "]" << C_RESET()
				<< "  " << C_WHITE() << script << C_RESET()
				<< "  " << statusStr(code)
				<< std::endl;
}

void	Logger::cgiTimeout(int fd, const std::string& script)
{
	std::cout << badge(BG_ORANGE(), "CGI")
				<< C_ORANGE() << "timeout" << C_RESET()
				<< C_GRAY() << "  fd[" << fd << "]" << C_RESET()
				<< "  " << C_WHITE() << script << C_RESET()
				<< "  " << statusStr(HTTP_GATEWAY_TIMEOUT)
				<< std::endl;
}

void	Logger::cgiError(int fd, const std::string& script, HttpStatus code)
{
	std::cout << badge(BG_RED(), "CGI")
				<< C_RED() << "error  " << C_RESET()
				<< C_GRAY() << "  fd[" << fd << "]" << C_RESET()
				<< "  " << C_WHITE() << script << C_RESET()
				<< "  " << statusStr(code)
				<< std::endl;
}

void	Logger::error(const std::string& msg)
{
	std::cout << C_BOLD() << badge(BG_RED(), "ERROR")
				<< C_RED() << msg << C_RESET()
				<< std::endl;
}
