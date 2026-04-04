#include <cstddef>
#include <cstring>
extern char **environ;

#include <iostream>

int main()
{
	std::string path;

	for (int i=0; environ[i]; i++)
	{
		if (!strncmp(environ[i], "PATH=", 5))
			path = environ[i];
	}
	size_t pos = path.find("/usr/");
	if (pos == std::string::npos)
		path = "PATH=/usr/local/bin:/usr/bin:/bin";
	else
	{
		path = "PATH=" + path.substr(pos);
		std::cout << path << std::endl;
	}
	return 0;
}
