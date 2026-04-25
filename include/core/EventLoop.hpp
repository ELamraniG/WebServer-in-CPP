#pragma once

#include "Client.hpp"
#include "Server.hpp"
#include "../cgi/CGIHandler.hpp"
#include "../http/RouteConfig.hpp"

#include <ctime>
#include <map>
#include <set>
#include <vector>
#include <cstddef>
#include <cstring>
#include <sys/poll.h>

const bool	VERBOSE = true;
const int	BUFFER_SIZE = 4096;
const int	PAUSE = 0;

class EventLoop
{
	private:
		std::vector<pollfd>					_pollFds;
		std::vector<Server*>				_serverList;
		std::vector<Server_block>			_serverBlocks;
		std::set<int>						_listeningFds;
		std::map<int, Client*>				_clientMap;
		std::map<int, CGIHandler*>			_cgiFdToHandler;
		std::map<int, Client*>				_cgiFdToClient;
		std::map<int, time_t>				_cgiStartTime;

		EventLoop();
		EventLoop(const EventLoop& obj);
		EventLoop& operator=(const EventLoop& obj);

		void		addToPoll(int fd, short event);
		void		removeFromPoll(size_t& i);
		void		handleNewClient(int fd);
		void		handleClientDisconnected(int fd, size_t& i, const std::string& msg);
		void		handleReadEvent(int fd, size_t& i);
		void		handleWriteEvent(int fd, size_t& i);
		bool		isTimeout(int fd);
		bool		isError(int fd, short revents) const;
		bool		isServer(int fd) const;
		void		handleError();
		void		logEvent(const std::string msg) const;
		bool		isReadable(const short revents) const;
		bool		isWritable(const short revents) const;
		std::string	extractExtention(const std::string& uri);
		std::string	extractCleanUri(const std::string& uri);
		bool		isCGIRequest(const std::string& uri, const std::map<std::string, std::string>& cgiPass);
		bool		resolveCGI(const std::string& uri, const RouteConfig& route, std::string& scriptPath, std::string& interpreter);
		bool		startCGI(int clientFd, const HTTPRequest& req, const RouteConfig& route);
		void		handleCGIRead(int readFd, size_t& i);
		void		handleCGIWrite(int writeFd, size_t& i);
		bool		isCGITimeout(int fd) const;
		void		handleCGITimeout(int fd, size_t& i);
		void		handleRequestComplete(int fd, size_t& i, const HTTPRequest& req, const RouteConfig& routeConfig);

	public:
		EventLoop(const std::vector<Server*>& servers, const std::vector<Server_block> serverBlocks);
		~EventLoop();

		static const int	POLL_TIMEOUT;
		static const int	CGI_TIMEOUT;

		void				run();
};
