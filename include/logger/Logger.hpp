#pragma once

#include "../core/ServerConstants.hpp"

#include <string>

class Logger
{
	public:
		static void	serverStart(const std::string& host, const std::string& port, int fd);
		static void	clientConnected(int fd, const std::string& ip, std::string& port);
		static void	clientDisconnected(int fd, HttpStatus code);
		static void	clientTimeout(int fd);
		static void	staticFile(const std::string& method, const std::string& uri, HttpStatus code);
		static void	cgiRun(int fd, const std::string& script);
		static void	cgiDone(int fd, const std::string& script, HttpStatus code);
		static void	cgiTimeout(int fd, const std::string& script);
		static void	cgiError(int fd, const std::string& script, HttpStatus code);
		static void	error(const std::string& msg);

	private:
		static std::string	colorOf(HttpStatus code);
		static std::string	labelOf(HttpStatus code);
		static std::string	statusStr(HttpStatus code);
		static std::string	badge(const std::string& bg, const std::string& label);

		static std::string	C_RESET()	{ return "\033[0m"; }
		static std::string	C_BOLD()		{ return "\033[1m"; }
		static std::string	C_DIM()		{ return "\033[2m"; }
		static std::string	C_GREEN()	{ return "\033[38;2;80;200;120m"; }
		static std::string	C_CYAN()		{ return "\033[38;2;80;180;220m"; }
		static std::string	C_YELLOW()	{ return "\033[38;2;240;180;50m"; }
		static std::string	C_RED()		{ return "\033[38;2;220;70;70m"; }
		static std::string	C_MAGENTA()	{ return "\033[38;2;180;100;220m"; }
		static std::string	C_ORANGE()	{ return "\033[38;2;240;130;50m"; }
		static std::string	C_GRAY()		{ return "\033[38;2;120;120;130m"; }
		static std::string	C_WHITE()	{ return "\033[38;2;220;220;230m"; }
		static std::string	BG_GREEN()	{ return "\033[48;2;30;80;50m"; }
		static std::string	BG_CYAN()	{ return "\033[48;2;20;70;100m"; }
		static std::string	BG_YELLOW()	{ return "\033[48;2;90;70;10m"; }
		static std::string	BG_RED()		{ return "\033[48;2;90;25;25m"; }
		static std::string	BG_MAGENTA()	{ return "\033[48;2;70;30;100m"; }
		static std::string	BG_ORANGE()	{ return "\033[48;2;100;50;10m"; }
		static std::string	BG_GRAY()	{ return "\033[48;2;40;40;50m"; }
};
