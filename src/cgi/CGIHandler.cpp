#include "../../include/cgi/CGIHandler.hpp"
#include "../../include/core/ServerConstants.hpp"

#include <cctype>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <signal.h>

CGIHandler::CGIHandler(const std::string& path, const std::string& interpreter, const std::string& method,
						const std::string& queryString, const std::string& body,
						const std::map<std::string, std::string>& headers) :
	_pid(-1),
	_done(false),
	_error(false),
	_code(200),
	_scriptPath(path),
	_interpreter(interpreter),
	_method(method),
	_queryString(queryString),
	_body(body),
	_headers(headers)
{
	_pipeIn[0]  = _pipeIn[1]  = -1;
	_pipeOut[0] = _pipeOut[1] = -1;
	buildEnv();
}

const std::string	CGIHandler::getPathEnv() const
{
	std::string	path;

	path = std::getenv("PATH");
	if (path.empty())
		path = "/usr/local/bin:/usr/bin:/bin";
	return (path);
}

void	CGIHandler::buildEnv()
{
	std::map<std::string, std::string>::const_iterator	it;
	std::string											key;
	std::ostringstream									oss;

	_envStrings.push_back("REQUEST_METHOD=" + _method);
	_envStrings.push_back("QUERY_STRING=" + _queryString);
	_envStrings.push_back("SCRIPT_FILENAME=" + _scriptPath);
	_envStrings.push_back("SCRIPT_NAME=" + _scriptPath);
	_envStrings.push_back("SERVER_PROTOCOL=HTTP/1.0");
	_envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_envStrings.push_back("REDIRECT_STATUS=200");
	_envStrings.push_back("PATH=" + getPathEnv());
	if (_method == "POST")
	{
		if (_headers.count("content-type"))
			_envStrings.push_back("CONTENT_TYPE=" + _headers["content-type"]);
		if (_headers.count("transfer-encoding"))
		{
			oss << _body.size();
			_envStrings.push_back("CONTENT_LENGTH=" + oss.str());
		}
		else if (_headers.count("content-length"))
			_envStrings.push_back("CONTENT_LENGTH=" +  _headers["content-length"]);
	}
	for (it = _headers.begin(); it != _headers.end(); it++)
	{
		if (it->first == "content-type" || it->first == "content-length" || it->first == "transfer-encoding")
			continue ;
		key = "HTTP_";
		for (size_t i = 0; i < it->first.size(); i++)
		{
			if (it->first[i] == '-')
				key += '_';
			else
				key += std::toupper(it->first[i]);
		}
		_envStrings.push_back(key + "=" + it->second);
	}
	for (size_t i = 0; i < _envStrings.size(); i++)
		_envp.push_back(const_cast<char*>(_envStrings[i].c_str()));
	_envp.push_back(NULL);
}

void	CGIHandler::setCode(int code)
{
	_code = code;
}

bool	CGIHandler::setNonBlocking(int fd)
{
	return (fcntl(fd, F_SETFL, O_NONBLOCK) != -1);
}

bool	CGIHandler::openPipe(int pipeFd[2])
{
	if (pipe(pipeFd) == -1)
		return (false);
	if (!setNonBlocking(pipeFd[0]) || !setNonBlocking(pipeFd[1]))
		return (false);
	return (true);
}

bool	CGIHandler::openPipes()
{
	if (!openPipe(_pipeOut))
		return (false);
	if (_method == "POST" && !openPipe(_pipeIn))
		return (false);
	return (true);
}

void	CGIHandler::checkExitStatus()
{
	int	status;

	if (_pid > 0 && waitpid(_pid,& status, WNOHANG) > 0)
	{
		if (WIFEXITED(status)&& WEXITSTATUS(status) != 0)
			_error = true;
		_pid = -1;
	}
}

void	CGIHandler::writeBody()
{
	ssize_t	bytes;

	if (_pipeIn[1] == -1 || _body.empty())
		return ;
	bytes = write(_pipeIn[1], _body.c_str(), _body.size());
	if (bytes < 0)
	{
		_error = true;
		closePipe(_pipeIn[1]);
		return ;
	}
	_body.erase(0, bytes);
	if (_body.empty())
		closePipe(_pipeIn[1]);
}
void	CGIHandler::readOutput()
{
	char	buffer[BUFFER_SIZE];
	ssize_t	bytes;

	bytes = read(_pipeOut[0], buffer, BUFFER_SIZE);
	if (bytes > 0)
		_output.append(buffer, bytes);
	else if (bytes == 0)
	{
		_done = true;
		closePipe(_pipeOut[0]);
		checkExitStatus();
	}
	else
	{
		closePipe(_pipeOut[0]);
		_error = true;
	}
}

void	CGIHandler::closePipe(int& pipe)
{
	if (pipe != -1)
	{
		close(pipe);
		pipe = -1;
	}
}

void CGIHandler::runChild()
{
	std::vector<char*>	argv;
	const char*			bin;

	closePipe(_pipeIn[1]);
	closePipe(_pipeOut[0]);
	if (dup2(_pipeOut[1], STDOUT_FILENO) == -1)
		exit(1);
	if (_method == "POST" && dup2(_pipeIn[0], STDIN_FILENO) == -1)
		exit(1);
	closePipe(_pipeOut[1]);
	closePipe(_pipeIn[0]);

	if (!_interpreter.empty())
	{
		argv.push_back(const_cast<char*>(_interpreter.c_str()));
		argv.push_back(const_cast<char*>(_scriptPath.c_str()));
		bin = _interpreter.c_str();
	}
	else
	{
		argv.push_back(const_cast<char*>(_scriptPath.c_str()));
		bin = _scriptPath.c_str();
	}
	argv.push_back(NULL);
	execve(bin, argv.data(), _envp.data());
	exit(1);
}

void	CGIHandler::runParent()
{
	closePipe(_pipeOut[1]);
	if (_method == "POST")
	{
		closePipe(_pipeIn[0]);
		writeBody();
	}
}
HttpStatus	CGIHandler::start()
{
	if (access(_scriptPath.c_str(), F_OK) == -1)
		return (HTTP_NOT_FOUND);
	if (access(_scriptPath.c_str(), X_OK) == -1)
		return (HTTP_FORBIDDEN);
	if (!openPipes() || (_pid = fork()) == -1)
		return (HTTP_INTERNAL_SERVER_ERROR);
	if (_pid == 0)
		runChild();
	else
		runParent();
	return (HTTP_OK);
}

std::string	CGIHandler::getOutput() const
{
	return (_output);
}

bool	CGIHandler::isWriteBodyDone() const
{
	return (_body.empty());
}

int	CGIHandler::getReadFd() const
{
	return (_pipeOut[0]);
}

int	CGIHandler::getWriteFd() const
{
	return (_pipeIn[1]);
}

int CGIHandler::getCode() const
{
	return (_code);
}

bool	CGIHandler::isDone() const
{
	return (_done);
}

bool	CGIHandler::isError() const
{
	return (_error);
}

void	CGIHandler::closeAllPipes()
{
	closePipe(_pipeIn[0]);
	closePipe(_pipeIn[1]);
	closePipe(_pipeOut[0]);
	closePipe(_pipeOut[1]);
}

void	CGIHandler::cleanup()
{
	if (_pid > 0)
	{
		kill(_pid, SIGKILL);
		waitpid(_pid, NULL, 0);
		_pid = -1;
		closeAllPipes();
	}
}

CGIHandler::~CGIHandler()
{
	if (_pid > 0)
		waitpid(_pid, NULL, 0);
	closeAllPipes();
}
