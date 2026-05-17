#include "../../include/core/EventLoop.hpp"
#include "../../include/core/ServerConstants.hpp"
#include "../../include/http/RequestParser.hpp"
#include "../../include/http/Router.hpp"
#include "../../include/http/ResponseBuilder.hpp"
#include "../../include/http/RouteConfig.hpp"
#include "../../include/http/MethodHandler.hpp"
#include "../../include/logger/Logger.hpp"

#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>
#include <sys/poll.h>

extern		bool g_running;
Logger		logger;

std::string	builderResponse(HttpStatus code, const RouteConfig& route)
{
	ResponseBuilder	builder;

	return (builder.build(MethodHandler::makeError(code, route)));
}

RouteConfig	EventLoop::getRoute(const Client* client)
{
	const location_block*	locationBlock;

	const Server_block& config = client->getServer()->getConfig();
	locationBlock = Router::match_location(client->httpReq, config);
	RouteConfig	route(config, locationBlock);
	return (route);
}

bool	EventLoop::isServer(int fd) const
{
	return (_listeningFds.count(fd));
}

bool	EventLoop::isReadable(const short revents) const
{
	return (revents & (POLLIN | POLLHUP));
}

bool	EventLoop::isWritable(const short revents) const
{
	return (revents & POLLOUT);
}

bool	EventLoop::isClientTimeout(int fd)
{
	return (!isServer(fd) && _clientMap.count(fd) && _clientMap[fd]->isTimedOut());
}

bool	EventLoop::isCGITimeout(int fd) const
{
	return (_cgiStartTime.count(fd) && (time(NULL) - _cgiStartTime.at(fd)) > CGI_TIMEOUT);
}

bool	EventLoop::isError(int fd, short revents) const
{
	return (!isServer(fd) && !_cgiFdToHandler.count(fd) && (revents & (POLLERR | POLLHUP)));
}

void	EventLoop::cgiCleanup(int fd, size_t& i)
{
	delete _cgiFdToHandler[fd];
	_cgiFdToHandler.erase(fd);
	_cgiStartTime.erase(fd);
	_cgiFdToClient.erase(fd);
	removeFromPoll(i);
}

void	EventLoop::registerCGI(int fd, CGIHandler* cgi)
{
	int	writeFd;
	int	readFd;

	readFd = cgi->getReadFd();
	writeFd = cgi->getWriteFd();
	_cgiStartTime[readFd] = time(NULL);
	_cgiFdToHandler[readFd] = cgi;
	_cgiFdToClient[readFd] = _clientMap[fd];
	addToPoll(readFd, POLLIN);
	if (writeFd != -1)
	{
		_cgiFdToHandler[writeFd] = cgi;
		_cgiFdToClient[writeFd] = _clientMap[fd];
		addToPoll(writeFd, POLLOUT);
	}
}

bool	EventLoop::startCGI(int clientFd, const RouteConfig& route)
{
	CGIHandler*	cgi;
	std::string	scriptPath;
	std::string	interpreter;
	HttpStatus	code;
	HTTPRequest	req;

	req = _clientMap[clientFd]->httpReq;
	CGIHandler::resolveCGI(req.getURI(), route, scriptPath);
	cgi = new CGIHandler(scriptPath, interpreter, req.getMethod(),
						req.getQueryString(), req.getBody(), req.getAllHeaders());
	logger.cgiRun(clientFd, req.getURI());
	code = cgi->start();
	if (code != HTTP_OK)
	{
		logger.cgiError(clientFd, scriptPath, code);
		_clientMap[clientFd]->setResponse(builderResponse(code, route));
		delete cgi;
		return (false);
	}
	registerCGI(clientFd, cgi);
	return (true);
}

void	EventLoop::addToPoll(int fd, short event)
{
	pollfd	pollFd;

	std::memset(&pollFd, 0, sizeof(pollFd));
	pollFd.fd = fd;
	pollFd.events = event;
	_pollFds.push_back(pollFd);
}

void	EventLoop::removeFromPoll(size_t &i)
{
	_pollFds.erase(_pollFds.begin() + i);
	if (i > 0)
		i--;
}

void	EventLoop::cleanupCGIWriteFd(int writeFd)
{
	if (writeFd == -1)
		return ;
	for (size_t j = 0; j < _pollFds.size(); j++)
	{
		if (_pollFds[j].fd == writeFd)
		{
			_pollFds.erase(_pollFds.begin() + j);
			break ;
		}
	}
	_cgiFdToHandler.erase(writeFd);
	_cgiFdToClient.erase(writeFd);
}

void	EventLoop::changeEvent(int fd, short event)
{
	for (size_t i = 0; i < _pollFds.size(); i++)
	{
		if (fd == _pollFds[i].fd)
		{
			_pollFds[i].events = event;
			return ;
		}
	}
}

void	EventLoop::handleNewClient(int serverFd)
{
	int				clientFd;
	size_t			i;
	Server_block	config;

	clientFd = -1;
	for (i=0; i<_serverList.size(); i++)
	{
		if (_serverList[i]->getFd() == serverFd)
		{
			clientFd = _serverList[i]->accept();
			break ;
		}
	}
	if (clientFd != -1)
	{
		config = _serverList[i]->getConfig();
		logger.clientConnected(clientFd, config.host, config.port);
		addToPoll(clientFd, POLLIN);
		_clientMap[clientFd] = new Client(clientFd, _serverList[i]);
	}
}

void	EventLoop::handleClientDisconnected(int fd, size_t& i, HttpStatus code)
{
	if (code == HTTP_REQUEST_TIMEOUT)
		logger.clientTimeout(fd);
	logger.clientDisconnected(fd, code);
	delete _clientMap[fd];
	_clientMap.erase(fd);
	removeFromPoll(i);
}

void	EventLoop::handleCGITimeout(int fd, size_t& i)
{
	int	writeFd;

	writeFd	= _cgiFdToHandler[fd]->getWriteFd();
	logger.cgiTimeout(_cgiFdToClient[fd]->getFd(), _cgiFdToClient[fd]->httpReq.getURI());
	changeEvent(_cgiFdToClient[fd]->getFd(), POLLOUT);
	_cgiFdToHandler[fd]->cleanup();
	cleanupCGIWriteFd(writeFd);
	_cgiFdToClient[fd]->setResponse(builderResponse(HTTP_GATEWAY_TIMEOUT, getRoute(_cgiFdToClient[fd])));
	cgiCleanup(fd, i);
}

void	EventLoop::handleCGIRead(int readFd, size_t& i)
{
	CGIHandler*		cgi;
	Client*			client;
	ResponseBuilder	build;

	client = _cgiFdToClient[readFd];
	RouteConfig		route = getRoute(client);
	cgi	= _cgiFdToHandler[readFd];
	cgi->readOutput();
	if (cgi->isDone() || cgi->isError())
	{
		if (cgi->isError())
		{
			logger.cgiError(readFd, client->httpReq.getURI(), HTTP_INTERNAL_SERVER_ERROR);
			client->setResponse(builderResponse(HTTP_INTERNAL_SERVER_ERROR, route));
		}
		else
		{
			logger.cgiDone(client->getFd(), client->httpReq.getURI(), HTTP_OK);
			client->setResponse(build.buildCgiResponse(cgi->getOutput(), route));
		}
		changeEvent(client->getFd(), POLLOUT);
		cleanupCGIWriteFd(cgi->getWriteFd());
		cgiCleanup(readFd, i);
	}
}

Response	EventLoop::dispatchMethod(Client* client, const RouteConfig& route)
{
	std::string		method;
	MethodHandler	handler;
	Response		response;

	method = client->httpReq.getMethod();
	if (method == "GET")
		response = handler.handleGET(client->httpReq, route);
	else if (method == "POST")
		response = handler.handlePOST(client->httpReq, route);
	else if (method == "DELETE")
		response = handler.handleDELETE(client->httpReq, route);
	else
		throw std::runtime_error("[501] NOT IMPLEMENTED");
	return (response);
}

bool	EventLoop::handleCGIIfNeeded(int fd, size_t i, const RouteConfig& route)
{
	Client*	client;

	client = _clientMap[fd];
	if (client->httpReq.getIsCGI())
	{
		if (startCGI(fd, route))
			_pollFds[i].events = PAUSE;
		else
		{
			logger.error("[500] INTERNAL SERVER ERROR");
			_pollFds[i].events = POLLOUT;
		}
		return (true);
	}
	return (false);
}

void	EventLoop::handleRequestComplete(int fd, size_t i, const RouteConfig& route)
{
	ResponseBuilder	responseBuilder;
	std::string		method;
	Client*			client;
	Response		response;

	try
	{
		client = _clientMap[fd];
		method = client->httpReq.getMethod();
		response = dispatchMethod(client, route);
		if (handleCGIIfNeeded(fd, i, route))
			return (client->updateLastActivity());
		logger.staticFile(method, route.getLocationPath(), (HttpStatus)response.statusCode);
		client->setResponse(responseBuilder.build(response));
	}
	catch(const std::exception& e)
	{
		logger.error(e.what());
		client->setResponse(builderResponse(HTTP_NOT_IMPLEMENTED, route));
	}
	_pollFds[i].events = POLLOUT;
	client->updateLastActivity();
}

bool	EventLoop::checkRequestParsing(Client* client, size_t& i)
{
	RequestParser::ParsingStatus	status;
	RequestParser					reqParser;

	status = reqParser.parseRequest(client->getRequestBuffer(), client->httpReq);
	if (status == RequestParser::P_INCOMPLETE)
		return (false);
	if (status == RequestParser::P_ERROR)
	{
		logger.error("[400] BAD REQUEST");
		client->setResponse(builderResponse(HTTP_BAD_REQUEST, getRoute(client)));
		_pollFds[i].events = POLLOUT;
		return (false);
	}
	return (true);
}

void	EventLoop::handleClientRead(int fd, size_t& i)
{
	ssize_t	bytes;
	Client*	client;

	client = _clientMap[fd];
	bytes = client->readFromSocket();
	if (bytes < 0)
		return (handleClientDisconnected(fd, i, HTTP_INTERNAL_SERVER_ERROR));
	else if (bytes == 0)
		return (handleClientDisconnected(fd, i, HTTP_CLIENT_DISCONNECTED));
	if (checkRequestParsing(client, i))
		handleRequestComplete(fd, i, getRoute(client));
}

void	EventLoop::handleReadEvent(int fd, size_t& i)
{
	if (isServer(fd))
		handleNewClient(fd);
	else if (_cgiFdToHandler.count(fd))
		handleCGIRead(fd, i);
	else
		handleClientRead(fd, i);
}

void	EventLoop::handleCGIWrite(int writeFd, size_t& i)
{
	CGIHandler*	cgi;
	Client*		client;

	cgi = _cgiFdToHandler[writeFd];
	client = _cgiFdToClient[writeFd];
	if (cgi->isError() || cgi->isWriteBodyDone())
	{
		if (cgi->isError())
		{
			logger.error("[500] INTERNAL SERVER ERROR");
			client->setResponse(builderResponse(HTTP_INTERNAL_SERVER_ERROR, getRoute(client)));
		}
		_cgiFdToHandler.erase(writeFd);
		_cgiFdToClient.erase(writeFd);
		removeFromPoll(i);
	}
	else
		cgi->writeBody();
}

void	EventLoop::handleWriteEvent(int fd, size_t& i)
{
	ssize_t	bytes;

	if (_cgiFdToHandler.count(fd))
		handleCGIWrite(fd, i);
	else
	{
		bytes = _clientMap[fd]->writeToSocket();
		if (bytes < 0)
		{
			logger.error("[500] INTERNAL SERVER ERROR");
			handleClientDisconnected(fd, i, HTTP_INTERNAL_SERVER_ERROR);
		}
		else if (_clientMap[fd]->hasNoPendingWrite())
			handleClientDisconnected(fd, i, HTTP_CLIENT_DISCONNECTED);
	}
}

void	EventLoop::processEvents()
{
	int		fd;
	short	revents;

	for (size_t i = 0; i < _pollFds.size(); i++)
	{
		fd = _pollFds[i].fd;
		revents = _pollFds[i].revents;
		if (isCGITimeout(fd))
			handleCGITimeout(fd, i);
		else if (isClientTimeout(fd))
			handleClientDisconnected(fd, i, HTTP_REQUEST_TIMEOUT);
		else if (isError(fd, revents))
			handleClientDisconnected(fd, i, HTTP_INTERNAL_SERVER_ERROR);
		else if (isReadable(revents))
			handleReadEvent(fd, i);
		else if (isWritable(revents))
			handleWriteEvent(fd, i);
	}
}

void	EventLoop::run()
{
	int	ret;

	while (g_running)
	{
		ret = poll(_pollFds.data(), _pollFds.size(), POLL_TIMEOUT);
		if (!g_running)
			break ;
		if (ret < 0)
			throw std::runtime_error("Error: poll.");
		processEvents();
	}
}

EventLoop::EventLoop() {}

EventLoop::EventLoop(const std::vector<Server*>& serverList) :
	_serverList(serverList)
{
	int	fd;

	for (size_t i = 0; i < serverList.size(); i++)
	{
		fd = serverList[i]->getFd();
		_listeningFds.insert(fd);
		addToPoll(fd, POLLIN);
	}
}

EventLoop::EventLoop(const EventLoop& other)
{
	(void) other;
}

EventLoop& EventLoop::operator=(const EventLoop& other) 
{
	(void) other;
	return (*this);
}

EventLoop::~EventLoop()
{
	std::map<int, Client*>::iterator		it;
	std::map<int, CGIHandler*>::iterator	cit;
	std::set<CGIHandler*>					deleted;

	for (it = _clientMap.begin(); it != _clientMap.end(); it++)
		delete it->second;
	for (cit = _cgiFdToHandler.begin(); cit != _cgiFdToHandler.end(); cit++)
	{
		if (!deleted.count(cit->second))
		{
			deleted.insert(cit->second);
			delete cit->second;
		}
	}
}
