#include "../../include/core/SocketGuard.hpp"

#include <unistd.h>

SocketGuard::SocketGuard() : fd(-1) {}

SocketGuard::SocketGuard(const SocketGuard& other)
{
	(void) other;
}

SocketGuard& SocketGuard::operator=(const SocketGuard& other)
{
	(void) other;
	return (*this);
}

SocketGuard::~SocketGuard()
{
	if (fd >= 0)
		close(fd);
}
