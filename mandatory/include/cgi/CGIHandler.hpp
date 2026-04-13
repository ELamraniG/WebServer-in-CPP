#pragma once

#include <string>
#include <map>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

extern char	**environ;

class CGIHandler
{
	private:
		CGIHandler();
		CGIHandler(const CGIHandler &obj);
		CGIHandler& operator=(const CGIHandler &obj);

		int									_pipeIn[2];
		int									_pipeOut[2];
		pid_t								_pid;
		bool								_done;
		bool								_error;
		std::string							_output;
		std::string							_scriptPath;
		std::string							_method;
		std::string							_queryString;
		std::string							_body;
		std::map<std::string, std::string>  _headers;
		std::vector<std::string>			_envStrings;
		std::vector<char*>					_envp;

		void        buildEnv();
		bool        openPipes();
		void		closePipe(int &pipe);
		void		checkExistStatus();
		std::string getPathEnv() const;
		void		runChild();
		void		runParent();
		std::string	getExtension();

	public:
		CGIHandler(const std::string &path, const std::string &method,
					const std::string &queryString, const std::string &body,
					const std::map<std::string, std::string> &headers);
		~CGIHandler();

		bool		start();
		int			getReadFd() const;
		int			getWriteFd() const;
		bool		isWriteBodyDone() const;
		bool		isDone() const;
		bool		isError() const;
		void		readOutput();
		void        writeBody();
		void		cleanup();
		std::string	getOutput() const;
};
