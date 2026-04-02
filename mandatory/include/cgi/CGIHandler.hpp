#pragma once

#include <string>
#include <map>

class CGIHandler
{
	private:
		CGIHandler();
		CGIHandler(const CGIHandler &obj);
		CGIHandler& operator=(const CGIHandler &obj);
		int			_pipeIn[2];
		int 		_pipeOut[2];
		std::string	_output;

	public:
		// CGIHandler(std::string &path, std::string &method, std::string &queryString, std::string &body, std::map<std::string, std::string> headers);
		~CGIHandler();
};
