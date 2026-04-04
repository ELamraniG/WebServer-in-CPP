#pragma once

#include <string>
#include <map>
#include <vector>

extern char **environ;

class CGIHandler
{
	private:
		CGIHandler();
		CGIHandler(const CGIHandler &obj);
		CGIHandler& operator=(const CGIHandler &obj);
		int									_pipeIn[2];
		int 								_pipeOut[2];
		std::string							_output;
		std::string							_scriptPath;
		std::string							_method;
		std::string							_queryString;
		std::string							_body;
		std::vector<std::string>			_envStrings;
		std::map<std::string, std::string>	_headers;
		void buildEnv();

	public:
		CGIHandler(std::string &path, std::string &method, std::string &queryString, std::string &body, std::map<std::string, std::string> headers);
		bool isCGIRequest() const;
		
		~CGIHandler();
};
