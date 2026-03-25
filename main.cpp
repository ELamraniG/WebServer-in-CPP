
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <iostream>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <poll.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int error(const char *msg)
{
	perror(msg);
	return (1);
}

int main()
{
	int server_fd;
	int client_fd;
	std::string buffer;
	buffer.resize(BUFFER_SIZE);
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int opt = 1;

	// create socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
		return (error("socket"));
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	// setup server address
	std::memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	// bind server to an address
	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		close(server_fd);
		return (error("bind"));
	}

	// listen
	if (listen(server_fd, 10) < 0)
	{
		close(server_fd);
		return (error("listen"));
	}

	std::cout << "Server listening on port " << PORT << std::endl;

	// read from client socket
	std::string response("HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!");
	std::string receiver;
	std::vector<pollfd>fds;
	pollfd server_poll;
	server_poll.fd = server_fd;
	server_poll.events = POLLIN;
	fds.push_back(server_poll);
	while (true)
	{
		int ret = poll(fds.data(), fds.size(), -1);
		if (ret < 0)
		{
			close(server_fd);
			return (error("poll"));
		}
		for (int i=0; i<fds.size(); i++)
		{
			if (fds[i].revents & POLLIN)
			{
				if (fds[i].fd == server_fd)
				{
					// accept
					client_fd = accept(server_fd,  (struct sockaddr *)&client_addr, &client_len);
					if (client_fd < 0)
					{
						close(server_fd);
						return (error("accept"));
					}
					receiver.clear();
					pollfd client_poll;
					client_poll.fd = client_fd;
					client_poll.events = POLLIN;
					fds.push_back(client_poll);
					std::cout << "Client connect" << std::endl;
				}
				else
				{
					while (true)
					{
						int bytes = read(fds[i].fd, &buffer[0], BUFFER_SIZE);
						if (bytes > 0)
						{
							receiver.append(buffer.c_str(), bytes);
							if (receiver.find("\r\n\r\n") != std::string::npos)
								break ;
						}
						else if (bytes == 0)
						{
							std::cout << "Client disconnected" << std::endl;
							break ;
						}
						else
						{
							close(server_fd);
							close(fds[i].fd);
							return (error("read"));
						}
					}
					write(fds[i].fd, response.c_str(), response.size());
					close(fds[i].fd);
					fds.erase(fds.begin() + i);
					i--;
				}
			}
		}
	}
	close(server_fd);
	return (0);
}
