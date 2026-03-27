
#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
#include <netinet/in.h>
#include <iostream>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <set>
#include <poll.h>
#include <fcntl.h>

// TODO: i should get ports and body_size_max from config file

#define BUFFER_SIZE 4096
#define TIMEOUT 50
#define MAX_HEADER_SIZE 8192

std::string response("HTTP/1.0 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!");

void cleanup(std::map<int, std::string> &receivers, std::map<int, std::string> &senders, std::vector<pollfd> &fds, int &i)
{
	close(fds[i].fd);
	receivers.erase(fds[i].fd);
	senders.erase(fds[i].fd);
	fds.erase(fds.begin() + i);
	i--;
}

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

	std::vector<int> ports;
	ports.push_back(8080);
	ports.push_back(8081);
	ports.push_back(8082);
	std::vector<pollfd>fds;
	std::set<int>server_fds;
	pollfd server_poll;
	for (int i=0; i<ports.size(); i++)
	{
		// 1- create socket
		server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd < 0)
			return (error("socket"));
		if (fcntl(server_fd, F_SETFL, O_NONBLOCK) == -1)
		{
			close(server_fd);
			return (error("fcntl(F_SETFL)"));
		}
		setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		// 2- bind it
		std::memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(ports[i]);
		server_addr.sin_addr.s_addr = INADDR_ANY;

		if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
		{
			close(server_fd);
			return (error("bind"));
		}

		// 3- start listening
		if (listen(server_fd, SOMAXCONN) < 0)
		{
			close(server_fd);
			return (error("listen"));
		}

		// 4- add it to server_fds and all_fds
		server_fds.insert(server_fd);
		server_poll.fd = server_fd;
		server_poll.events = POLLIN;
		fds.push_back(server_poll);
	}

	std::cout << "Server listening on ports: " << ports[0] << ", " << ports[1] << ", " << ports[2] << std::endl;

	std::map<int, std::string> receivers;
	std::map<int, std::string> senders;
	std::map<int, time_t> last_activity;
	while (true)
	{
		int ret = poll(fds.data(), fds.size(), 5000);
		if (ret < 0)
		{
			for (int i=0; i<fds.size(); i++)
				close(fds[i].fd);
			return (error("poll"));
		}
		for (int i=0; i<fds.size(); i++)
		{
			if (!server_fds.count(fds[i].fd))
			{
				if (time(NULL) - last_activity[fds[i].fd] > TIMEOUT)
				{
					last_activity.erase(fds[i].fd);
					cleanup(receivers, senders, fds, i);
					std::cout << "Client timed out" << std::endl;
					continue;
				}
			}
			if (!server_fds.count(fds[i].fd) && (fds[i].revents & (POLLERR | POLLHUP)))
				cleanup(receivers, senders, fds, i);
			else if (fds[i].revents & POLLIN)
			{
				if (server_fds.count(fds[i].fd))
				{
					client_fd = accept(fds[i].fd,  (struct sockaddr *)&client_addr, &client_len);
					if (client_fd < 0)
					{
						perror("accept");
						continue ;
					}
					else if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
					{
						perror("fcntl(F_SETFL)");
						close(client_fd);
						continue ;
					}	
					pollfd client_poll;
					client_poll.fd = client_fd;
					client_poll.events = POLLIN;
					fds.push_back(client_poll);
					receivers[client_fd] = "";
					last_activity[client_fd] = time(NULL);
					std::cout << "Client connect" << std::endl;
				}
				else
				{
					int bytes = read(fds[i].fd, &buffer[0], BUFFER_SIZE);
					if (bytes > 0)
					{
						if (receivers[fds[i].fd].size() + bytes > MAX_HEADER_SIZE)
						{
							cleanup(receivers, senders, fds, i);
							continue ;
						}
						receivers[fds[i].fd].append(buffer.c_str(), bytes);
						if (receivers[fds[i].fd].find("\r\n\r\n") != std::string::npos)
						{
							fds[i].events = POLLOUT;
							senders[fds[i].fd] = response;
						}
						last_activity[fds[i].fd] = time(NULL);
					}
					else if (bytes == 0)
					{
						std::cout << "Client disconnected" << std::endl;
						cleanup(receivers, senders, fds, i);
					}
					else
					{
						perror("read");
						cleanup(receivers, senders, fds, i);
					}
				}
			}
			else if (fds[i].revents & POLLOUT)
			{
				int response_size = senders[fds[i].fd].size();
				int bytes = write(fds[i].fd, senders[fds[i].fd].c_str(), response_size);
				if (bytes < 0)
				{
					perror("write");
					cleanup(receivers, senders, fds, i);
				}
				else if (bytes == 0)
				{
					std::cout << "Client disconnected" << std::endl;
					cleanup(receivers, senders, fds, i);
				}
				else if (bytes == response_size)
					cleanup(receivers, senders, fds, i);
				else
					senders[fds[i].fd].erase(0, bytes);
			}
		}
	}
	return (0);
}
