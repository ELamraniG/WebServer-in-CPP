#include "../../include/core/EventLoop.hpp"
#include "../../include/core/ServerConstants.hpp"
#include "../../include/http/MethodHandler.hpp"
#include "../../include/http/RequestParser.hpp"
#include "../../include/http/ResponseBuilder.hpp"
#include "../../include/http/RouteConfig.hpp"
#include "../../include/http/Router.hpp"
#include "../../include/logger/Logger.hpp"

#include <cstddef>
#include <stdexcept>

extern bool g_running;
Logger logger;

std::string builderResponse(HttpStatus code, const RouteConfig &route) {
  ResponseBuilder builder;

  return (builder.build(MethodHandler::makeError(code, "Error", route)));
}

RouteConfig EventLoop::getRoute(const Client *client) {
  location_block *locationBlock;

  Server_block &serverBlock =
      Router::match_server(client->httpReq, _serverBlocks);
  locationBlock = Router::match_location(client->httpReq, serverBlock);
  RouteConfig route(serverBlock, locationBlock);
  return (route);
}

bool EventLoop::isServer(int fd) const { return (_listeningFds.count(fd)); }

bool EventLoop::isReadable(const short revents) const {
  return (revents & (POLLIN | POLLHUP));
}

bool EventLoop::isWritable(const short revents) const {
  return (revents & POLLOUT);
}

bool EventLoop::isTimeout(int fd) {
  return (!isServer(fd) && _clientMap.count(fd) &&
          _clientMap[fd]->isTimedOut());
}

bool EventLoop::isCGITimeout(int fd) const {
  return (_cgiStartTime.count(fd) &&
          (time(NULL) - _cgiStartTime.at(fd)) > CGI_TIMEOUT);
}

bool EventLoop::isError(int fd, short revents) const {
  return (!isServer(fd) && !_cgiFdToHandler.count(fd) &&
          (revents & (POLLERR | POLLHUP)));
}

std::string EventLoop::extractExtention(const std::string &uri) {
  size_t dot;
  size_t queryStartAt;
  std::string extension;

  dot = uri.find_last_of('.');
  if (dot == std::string::npos)
    return ("");
  extension = uri.substr(dot);
  queryStartAt = extension.find('?');
  if (queryStartAt != std::string::npos)
    extension = extension.substr(0, queryStartAt);
  return (extension);
}

bool EventLoop::isCGIRequest(
    const std::string &uri, const std::map<std::string, std::string> &cgiPass) {
  std::string extension;

  extension = extractExtention(uri);
  if (extension.empty() || cgiPass.empty())
    return (false);
  return (cgiPass.count(extension));
}

std::string EventLoop::extractCleanUri(const std::string &uri) {
  std::string cleanUri;
  std::size_t queryStartAt;

  queryStartAt = uri.find('?');
  if (queryStartAt == std::string::npos)
    return (uri);
  cleanUri = uri.substr(0, queryStartAt);
  return (cleanUri);
}

bool EventLoop::resolveCGI(const std::string &uri, const RouteConfig &route,
                           std::string &scriptPath, std::string &interpreter) {
  std::string extension;
  const std::map<std::string, std::string> &cgiPass = route.getCgiPass();
  std::map<std::string, std::string>::const_iterator it;
  std::string cleanUri;
  std::string relativeUri;
  std::string root;
  std::string locationPath;

  extension = extractExtention(uri);
  it = cgiPass.find(extension);
  if (it == cgiPass.end())
    return (false);
  interpreter = it->second;
  cleanUri = extractCleanUri(uri);
  root = route.getRoot();
  locationPath = route.getLocationPath();
  relativeUri = cleanUri;
  if (!locationPath.empty() &&
      cleanUri.compare(0, locationPath.size(), locationPath) == 0)
    relativeUri = cleanUri.substr(locationPath.size());
  if (!root.empty() && root[root.size() - 1] != '/' && !relativeUri.empty() &&
      relativeUri[0] != '/')
    root += '/';
  scriptPath = root + relativeUri;
  return (true);
}

bool EventLoop::startCGI(int clientFd, const RouteConfig &route) {
  int writeFd;
  int readFd;
  CGIHandler *cgi;
  std::string scriptPath;
  std::string interpreter;
  HttpStatus code;
  HTTPRequest req;

  req = _clientMap[clientFd]->httpReq;
  if (!resolveCGI(req.getURI(), route, scriptPath, interpreter))
    return (false);
  cgi =
      new CGIHandler(scriptPath, interpreter, req.getMethod(),
                     req.getQueryString(), req.getBody(), req.getAllHeaders());
  logger.cgiRun(clientFd, req.getURI());
  code = cgi->start();
  if (code != HTTP_OK) {
    logger.cgiError(clientFd, scriptPath, code);
    _clientMap[clientFd]->setResponse(builderResponse(code, route));
    delete cgi;
    return (false);
  }
  readFd = cgi->getReadFd();
  writeFd = cgi->getWriteFd();
  _cgiStartTime[readFd] = time(NULL);
  _cgiFdToHandler[readFd] = cgi;
  _cgiFdToClient[readFd] = _clientMap[clientFd];
  addToPoll(readFd, POLLIN);
  if (writeFd != -1) {
    _cgiFdToHandler[writeFd] = cgi;
    _cgiFdToClient[writeFd] = _clientMap[clientFd];
    addToPoll(writeFd, POLLOUT);
  }
  return (true);
}

void EventLoop::addToPoll(int fd, short event) {
  pollfd pollFd;

  std::memset(&pollFd, 0, sizeof(pollFd));
  pollFd.fd = fd;
  pollFd.events = event;
  _pollFds.push_back(pollFd);
}

void EventLoop::removeFromPoll(size_t &i) {
  _pollFds.erase(_pollFds.begin() + i);
  if (i > 0)
    i--;
}

void EventLoop::handleNewClient(int serverFd) {
  int clientFd;
  size_t i;

  clientFd = -1;
  for (i = 0; i < _serverList.size(); i++) {
    if (_serverList[i]->getFd() == serverFd) {
      clientFd = _serverList[i]->accept();
      break;
    }
  }
  if (clientFd != -1) {
    logger.clientConnected(clientFd, _serverBlocks[i].host,
                           _serverBlocks[i].port);
    addToPoll(clientFd, POLLIN);
    _clientMap[clientFd] = new Client(clientFd);
  }
}

void EventLoop::handleClientDisconnected(int fd, size_t &i, HttpStatus code) {
  logger.clientDisconnected(fd, code);
  delete _clientMap[fd];
  _clientMap.erase(fd);
  removeFromPoll(i);
}

// void	EventLoop::cleanupCGIWriteFd(int writeFd)
// {
// 	if (writeFd == -1)
// 		return ;
// 	for (size_t j = 0; j < _pollFds.size(); j++)
// 	{
// 		if (_pollFds[j].fd == writeFd)
// 		{
// 			_pollFds.erase(_pollFds.begin() + j);
// 			break ;
// 		}
// 	}
// 	_cgiFdToHandler.erase(writeFd);
// 	_cgiFdToClient.erase(writeFd);
// }

void EventLoop::handleCGITimeout(int fd, size_t &i) {
  logger.cgiTimeout(_cgiFdToClient[fd]->getFd(),
                    _cgiFdToClient[fd]->httpReq.getURI());
  for (size_t j = 0; j < _pollFds.size(); j++) {
    if (_cgiFdToClient[fd]->getFd() == _pollFds[j].fd) {
      _pollFds[j].events = POLLOUT;
      break;
    }
  }
  // int	writeFd = _cgiFdToHandler[fd]->getWriteFd();
  _cgiFdToHandler[fd]->cleanup();
  // cleanupCGIWriteFd(writeFd);
  _cgiFdToClient[fd]->setResponse(
      builderResponse(HTTP_GATEWAY_TIMEOUT, getRoute(_cgiFdToClient[fd])));
  delete _cgiFdToHandler[fd];
  _cgiFdToHandler.erase(fd);
  _cgiStartTime.erase(fd);
  _cgiFdToClient.erase(fd);
  removeFromPoll(i);
}

void EventLoop::handleCGIRead(int readFd, size_t &i) {
  CGIHandler *cgi;
  Client *client;
  ResponseBuilder build;
  location_block *locationBlock;

  cgi = _cgiFdToHandler[readFd];
  client = _cgiFdToClient[readFd];
  cgi->readOutput();
  if (cgi->isDone() || cgi->isError()) {
    Server_block &serverBlock =
        Router::match_server(client->httpReq, _serverBlocks);
    locationBlock = Router::match_location(client->httpReq, serverBlock);
    RouteConfig route(serverBlock, locationBlock);
    if (cgi->isError()) {
      logger.cgiError(readFd, client->httpReq.getURI(),
                      HTTP_INTERNAL_SERVER_ERROR);
      client->setResponse(builderResponse(HTTP_INTERNAL_SERVER_ERROR, route));
    } else {
      logger.cgiDone(client->getFd(), client->httpReq.getURI(), HTTP_OK);
      client->setResponse(build.buildCgiResponse(cgi->getOutput(), route));
    }
    for (size_t j = 0; j < _pollFds.size(); j++) {
      if (_pollFds[j].fd == client->getFd()) {
        _pollFds[j].events = POLLOUT;
        break;
      }
    }
    // int	writeFd = cgi->getWriteFd();
    // cleanupCGIWriteFd(writeFd);
    removeFromPoll(i);
    delete cgi;
    _cgiFdToHandler.erase(readFd);
    _cgiFdToClient.erase(readFd);
    _cgiStartTime.erase(readFd);
  }
}

void EventLoop::handleRequestComplete(int fd, size_t &i,
                                      const RouteConfig &route) {
  MethodHandler handler;
  ResponseBuilder responseBuilder;
  std::string method;
  Client *client;
  Response response;

  client = _clientMap[fd];
  method = client->httpReq.getMethod();
  if (method == "GET")
    response = handler.handleGET(client->httpReq, route);
  else if (method == "POST")
    response = handler.handlePOST(client->httpReq, route);
  else if (method == "DELETE")
    response = handler.handleDELETE(client->httpReq, route);
  else {
    logger.error("Not Implemented");
    _clientMap[fd]->setResponse(builderResponse(HTTP_NOT_IMPLEMENTED, route));
    _pollFds[i].events = POLLOUT;
    _clientMap[fd]->updateLastActivity();
    return;
  }
  if (client->httpReq.getIsCGI()) {
    if (startCGI(fd, route))
      _pollFds[i].events = PAUSE;
    else {
      logger.error("INTERNAL_SERVER_ERROR");
      _pollFds[i].events = POLLOUT;
    }
  } else {
    logger.staticFile(method, route.getLocationPath(),
                      (HttpStatus)response.statusCode);
    client->setResponse(responseBuilder.build(response));
    _pollFds[i].events = POLLOUT;
  }
  _clientMap[fd]->updateLastActivity();
}

void EventLoop::handleReadEvent(int fd, size_t &i) {
  ssize_t bytes;
  RequestParser reqParser;
  RequestParser::ParsingStatus status;
  location_block *locationBlock;

  if (isServer(fd))
    handleNewClient(fd);
  else if (_cgiFdToHandler.count(fd))
    handleCGIRead(fd, i);
  else {
    bytes = _clientMap[fd]->readFromSocket();
    if (bytes < 0)
      handleClientDisconnected(fd, i, HTTP_INTERNAL_SERVER_ERROR);
    else if (bytes == 0)
      handleClientDisconnected(fd, i, HTTP_CLIENT_DISCONNECTED);
    else {
      status = reqParser.parseRequest(_clientMap[fd]->getRequestBuffer(),
                                      _clientMap[fd]->httpReq);
      if (status == RequestParser::P_INCOMPLETE)
        return;
      if (status == RequestParser::P_ERROR) {
        logger.error("BAD REQUEST");
        _clientMap[fd]->setResponse(
            builderResponse(HTTP_BAD_REQUEST, getRoute(_clientMap[fd])));
        _pollFds[i].events = POLLOUT;
        return;
      }
      Server_block &serverBlock =
          Router::match_server(_clientMap[fd]->httpReq, _serverBlocks);
      locationBlock =
          Router::match_location(_clientMap[fd]->httpReq, serverBlock);
      RouteConfig route(serverBlock, locationBlock);
      handleRequestComplete(fd, i, route);
    }
  }
}

<<<<<<< HEAD
void EventLoop::handleCGIWrite(int writeFd, size_t &i) {
  CGIHandler *cgi;

  cgi = _cgiFdToHandler[writeFd];
  if (cgi->isError() || cgi->isWriteBodyDone()) {
    if (cgi->isError()) {
      logger.error("INTERNAL_SERVER_ERROR");
      _cgiFdToClient[writeFd]->setResponse(builderResponse(
          HTTP_INTERNAL_SERVER_ERROR, getRoute(_cgiFdToClient[writeFd])));
    }
    _cgiFdToHandler.erase(writeFd);
    _cgiFdToClient.erase(writeFd);
    removeFromPoll(i);
  } else
    cgi->writeBody();
=======
void	EventLoop::handleReadEvent(int fd, size_t& i)
{
	ssize_t							bytes;
	RequestParser					reqParser;
	RequestParser::ParsingStatus	status;

	if (isServer(fd))
		handleNewClient(fd);
	else if (_cgiFdToHandler.count(fd))
		handleCGIRead(fd, i);
	else
	{
		bytes = _clientMap[fd]->readFromSocket();
		if (bytes < 0)
			handleClientDisconnected(fd, i, HTTP_INTERNAL_SERVER_ERROR);
		else if (bytes == 0)
			handleClientDisconnected(fd, i, HTTP_CLIENT_DISCONNECTED);
		else
		{
			status = reqParser.parseRequest(_clientMap[fd]->getRequestBuffer(), _clientMap[fd]->httpReq);
			if (status == RequestParser::P_INCOMPLETE)
				return ;
			if (status == RequestParser::P_ERROR)
			{
				logger.error("BAD REQUEST");
				_clientMap[fd]->setResponse(builderResponse(HTTP_BAD_REQUEST, getRoute(_clientMap[fd])));
				_pollFds[i].events = POLLOUT;
				return ;
			}
			handleRequestComplete(fd, i, getRoute(_clientMap[fd]));
		}
	}
>>>>>>> 7ea85ecf4e27598039fc8e300998f5527edce25a
}

void EventLoop::handleWriteEvent(int fd, size_t &i) {
  ssize_t bytes;

  if (_cgiFdToHandler.count(fd))
    handleCGIWrite(fd, i);
  else {
    bytes = _clientMap[fd]->writeToSocket();
    if (bytes < 0) {
      logger.error("INTERNAL_SERVER_ERROR");
      handleClientDisconnected(fd, i, HTTP_INTERNAL_SERVER_ERROR);
    } else if (_clientMap[fd]->hasNoPendingWrite())
      handleClientDisconnected(fd, i, HTTP_CLIENT_DISCONNECTED);
  }
}

void EventLoop::run() {
  int fd;
  short revents;

  while (g_running) {
    if (poll(_pollFds.data(), _pollFds.size(), POLL_TIMEOUT) < 0) {
      if (!g_running)
        break;
      throw std::runtime_error("Error: poll.");
    }
    for (size_t i = 0; i < _pollFds.size(); i++) {
      fd = _pollFds[i].fd;
      revents = _pollFds[i].revents;
      if (isCGITimeout(fd))
        handleCGITimeout(fd, i);
      else if (isTimeout(fd))
        handleClientDisconnected(fd, i, HTTP_REQUEST_TIMEOUT);
      else if (isError(fd, revents))
        handleClientDisconnected(fd, i, HTTP_INTERNAL_SERVER_ERROR);
      else if (isReadable(revents))
        handleReadEvent(fd, i);
      else if (isWritable(revents))
        handleWriteEvent(fd, i);
    }
  }
}

EventLoop::EventLoop() {}

EventLoop::EventLoop(const std::vector<Server *> &serverList,
                     const std::vector<Server_block> serverBlocks)
    : _serverList(serverList), _serverBlocks(serverBlocks) {
  int fd;

  for (size_t i = 0; i < serverList.size(); i++) {
    fd = serverList[i]->getFd();
    _listeningFds.insert(fd);
    addToPoll(fd, POLLIN);
  }
}

EventLoop::EventLoop(const EventLoop &obj) { (void)obj; }

EventLoop &EventLoop::operator=(const EventLoop &obj) {
  (void)obj;
  return (*this);
}

EventLoop::~EventLoop() {
  std::map<int, Client *>::iterator it;
  std::map<int, CGIHandler *>::iterator cit;
  std::set<CGIHandler *> deleted;

  for (it = _clientMap.begin(); it != _clientMap.end(); it++)
    delete it->second;
  for (cit = _cgiFdToHandler.begin(); cit != _cgiFdToHandler.end(); cit++) {
    if (!deleted.count(cit->second)) {
      deleted.insert(cit->second);
      delete cit->second;
    }
  }
}
