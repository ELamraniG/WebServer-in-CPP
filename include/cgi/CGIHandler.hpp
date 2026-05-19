#pragma once

#include "../core/ServerConstants.hpp"
#include "../../include/http/RouteConfig.hpp"

#include <string>
#include <map>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

extern char** environ;

class CGIHandler
{
	private:
		CGIHandler();
		CGIHandler(const CGIHandler& other);
		CGIHandler& operator=(const CGIHandler& other);

		int									_pipeIn[2];
		int									_pipeOut[2];
		int									_openedFds[4];
		pid_t								_pid;
		bool								_done;
		bool								_error;
		std::string							_output;
		std::string							_scriptPath;
		std::string							_interpreter;
		std::string							_method;
		std::string							_queryString;
		std::string							_body;
		std::map<std::string, std::string>  _headers;
		std::vector<std::string>			_envStrings;
		std::vector<char*>					_envp;

		void				buildEnv();
		void				addBaseEnv();
		void				addPostEnv();
		void				addHeaderEnv();
		bool				isSpecialHeader(const std::string& key);
		std::string			formatHeaderKey(const std::string& key);
		void				buildEnvp();
		bool				setNonBlocking(int fd);
		bool				openPipe(int pipeFd[2]);
		bool				openPipes();
		void				closePipe(int& pipe);
		void				closeAllPipes();
		void				checkExitStatus();
		const std::string	getPathEnv() const;
		void				runChild();
		void				setupPipes();
		std::vector<char*>	buildArgv();
		void				execProgram(const std::vector<char*>& argv);
		void				runParent();
		const std::string	getExtension();
		bool				isOpenedPipe(int fd) const;
		void				closeInheritedFds();
		void				saveOpenedFds();

	public:
		CGIHandler(const std::string& path, const std::string& interpreter, const std::string& method,
					const std::string& queryString, const std::string& body,
					const std::map<std::string, std::string>& headers);
		~CGIHandler();

		HttpStatus			start();
		int					getReadFd() const;
		int					getWriteFd() const;
		std::string			getOutput() const;
		bool				isWriteBodyDone() const;
		bool				isDone() const;
		bool				isError() const;
		void				readOutput();
		void		        writeBody();
		void				cleanup();
		static std::string	extractExtention(const std::string& uri);
		static bool			isCGIRequest(const HTTPRequest& request, const RouteConfig& route);
		static std::string	extractCleanUri(const std::string& uri);
		static void			resolveCGI(const std::string& uri, const RouteConfig& route, std::string& scriptPath);
		
};
