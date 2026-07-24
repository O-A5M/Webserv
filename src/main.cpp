#include <csignal>
#include <cstring>

#include "configParser.hpp"
#include "serverConfig.hpp"
#include "Server.hpp"
#include "EventLoop.hpp"
#include <iostream>
#include <vector>
#include "Server_Handler.hpp"


static void handleShutdownSignal(int) {
	EventLoop::running = 0;
}

int main(int ac, char **av)
{
	if (ac != 2)
	{
		std::cerr << "Usage: " << av[0] << " <config_file>" << std::endl;
		return 1;
	}

	struct sigaction sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handleShutdownSignal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	signal(SIGPIPE, SIG_IGN);

	try
	{
		ConfigParser parser(av[1]);
		std::vector<ServerConfig> serverConf = parser.getServers();

		EventLoop	loop;
		std::vector<Server>	servers;
		servers.reserve(serverConf.size());
		for (int i = 0; i < static_cast<int>(serverConf.size()); i++) {
			servers.push_back(Server(serverConf[i]));
			servers[i].initialize_socket();
			new ServerHandler(servers[i].GetFd(), servers[i].GetConfig(), loop);
		}
		loop.Loop();
		loop.shutdown();
	}
	catch (const std::exception &e)
	{
		std::cerr << "ERROR: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
