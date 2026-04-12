#include "../../include/cgi/CGIHandler.hpp"

#include <cctype>
#include <cstring>
#include <cstdlib>

static const int	BUFFER_SIZE = 4096;

CGIHandler::CGIHandler(const std::string &path, const std::string &method,
						const std::string &queryString, const std::string &body,
						const std::map<std::string, std::string> &headers) :
	_pid(-1),
	_done(false),
	_error(false),
	_scriptPath(path),
	_method(method),
	_queryString(queryString),
	_body(body),
	_headers(headers)
{
	_pipeIn[0]  = _pipeIn[1]  = -1;
	_pipeOut[0] = _pipeOut[1] = -1;
	buildEnv();
}

std::string	CGIHandler::getPathEnv() const
{
	for (int i=0; environ[i]; i++)
	{
		if (strncmp(environ[i], "PATH=", 5) == 0) // TODO: implement my strncmp
			return std::string(environ[i] + 5);
	}
	return ("/usr/local/bin:/usr/bin:/bin");
}

void	CGIHandler::buildEnv()
{
	std::map<std::string, std::string>::const_iterator	it;
	std::string											key;

	_envStrings.push_back("REQUEST_METHOD=" + _method);
	_envStrings.push_back("QUERY_STRING=" + _queryString);
	_envStrings.push_back("SCRIPT_FILENAME=" + _scriptPath);
	_envStrings.push_back("SCRIPT_NAME=" + _scriptPath);
	_envStrings.push_back("SERVER_PROTOCOL=HTTP/1.0");
	_envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_envStrings.push_back("PATH=" + getPathEnv());
	if (_method == "POST")
	{
		if (_headers.count("content-type"))
			_envStrings.push_back("CONTENT_TYPE=" + _headers["content-type"]);
		if (_headers.count("content-length"))
			_envStrings.push_back("CONTENT_LENGTH=" + _headers["content-length"]);
	}
	for (it = _headers.begin(); it != _headers.end(); ++it)
	{
		if (it->first == "content-type" || it->first == "content-length")
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

bool	CGIHandler::openPipes()
{
	if (pipe(_pipeOut) == -1)
		return (false);
	if (fcntl(_pipeOut[0], F_SETFL, O_NONBLOCK) == -1 || fcntl(_pipeOut[1], F_SETFL, O_NONBLOCK) == -1)
	{
		closePipe(_pipeOut[0]);
		closePipe(_pipeOut[1]);
		return (false);
	}
	if (_method == "POST")
	{
		if (pipe(_pipeIn) == -1)
		{
			closePipe(_pipeOut[0]);
			closePipe(_pipeOut[1]);
			return (false);
		}
		if (fcntl(_pipeIn[0], F_SETFL, O_NONBLOCK) == -1 || fcntl(_pipeIn[1], F_SETFL, O_NONBLOCK) == -1)
		{
			closePipe(_pipeOut[0]);
			closePipe(_pipeOut[1]);
			closePipe(_pipeIn[0]);
			closePipe(_pipeIn[1]);
			return (false);
		}
	}
	return (true);
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
	else if (bytes == 0 || bytes < 0)
	{
		if (bytes == 0)
			_done = true;
		else
			_error = true;
		closePipe(_pipeOut[0]);
	}
}

void	CGIHandler::closePipe(int &pipe)
{
	if (pipe != -1)
	{
		close(pipe);
		pipe = -1;
	}
}

void	CGIHandler::runChild()
{
	char	*argv[2];

	
	closePipe(_pipeIn[1]);
	closePipe(_pipeOut[0]);
	if (dup2(_pipeOut[1], STDOUT_FILENO) == -1)
		exit(1);
	if (_method == "POST")
	{
		if (dup2(_pipeIn[0], STDIN_FILENO) == -1)
			exit(1);
	}
	closePipe(_pipeOut[1]);
	closePipe(_pipeIn[0]);
	argv[0] = const_cast<char *>(_scriptPath.c_str());
	argv[1] = NULL;
	execve(_scriptPath.c_str(), argv, _envp.data());
	exit(1);
}

void	CGIHandler::runParent()
{
	closePipe(_pipeIn[0]);
	closePipe(_pipeOut[1]);
	if (_method == "POST")
		writeBody();
}

bool	CGIHandler::start()
{
	if (!openPipes() || (_pid = fork()) == -1)
		return (false);
	if (_pid == 0)
		runChild();
	else
		runParent();
	return (true);
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

bool	CGIHandler::isDone() const
{
	return (_done);
}

bool	CGIHandler::isError() const
{
	return (_error);
}

void	CGIHandler::cleanup()
{
	if (_pid > 0)
	{
		kill(_pid, SIGKILL);
		waitpid(_pid, NULL, 0);
		_pid = -1;
	}
}

CGIHandler::~CGIHandler()
{
	if (_pid > 0)
		waitpid(_pid, NULL, WNOHANG);
	closePipe(_pipeIn[0]);
	closePipe(_pipeIn[1]);
	closePipe(_pipeOut[0]);
	closePipe(_pipeOut[1]);
}
