#include "../../include/signals/SignalHandler.hpp"

#include <csignal>
#include <cstring>

bool	g_running = true;

void	signalHandler(int)
{
	g_running = false;
}

void	setupSignals()
{
	struct sigaction	sa;

	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, NULL);
}
