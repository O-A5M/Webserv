#include "configParser.hpp"
#include "serverConfig.hpp"
#include "Server.hpp"
#include "EventLoop.hpp"
#include <iostream>
#include <vector>
#include "Server_Handler.hpp"


int main(int ac, char **av)
{
	if (ac != 2)
	{
		std::cerr << "Error: Usage-> " << av[0] << " <config_file>" << std::endl;
		return 1;
	}

	try
	{
		// Parsing of the config file
		ConfigParser parser(av[1]);
		std::vector<ServerConfig> serverConf = parser.getServers();

		// Starting the multiplexer
		EventLoop	loop;
		// Server ser(serverConf[1]);
		// ser.initialize_socket();

		// Starting the connection for the servers
		std::vector<Server>	servers;
		servers.reserve(serverConf.size());
		for (int i = 0; i < static_cast<int>(serverConf.size()); i++) {
			servers.push_back(Server(serverConf[i]));
			servers[i].initialize_socket();
			// std::cout << "====>" << servers[i].GetFd() << std::endl;
			new ServerHandler(servers[i].GetFd(), servers[i].GetConfig(), loop);
		}
		loop.Loop();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Config parse error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}