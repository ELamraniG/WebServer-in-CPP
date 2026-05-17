#pragma once

class SocketGuard
{
	private:
		SocketGuard(const SocketGuard& other);
		SocketGuard& operator=(const SocketGuard& other);

	public:
		int	fd;

		SocketGuard();
		~SocketGuard();
};
