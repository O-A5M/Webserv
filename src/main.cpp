#include "configParser.hpp"
#include "serverConfig.hpp"
#include "Server.hpp"

#include <iostream>

int main(int ac, char **av)
{
	if (ac != 2)
	{
		std::cerr << "Usage: " << av[0] << " <config_file>" << std::endl;
		return 1;
	}

	try
	{
		ConfigParser parser(av[1]);
		const std::vector<ServerConfig> servers = parser.getServers();
		Server server(servers);
		server.initialize_socket();

		// for (std::size_t i = 0; i < servers.size(); ++i)
		// {
		// 	const ServerConfig &server = servers[i];

		// 	std::cout << "server[" << i << "]" << std::endl;
		// 	std::cout << "  host: " << server.host << std::endl;
		// 	std::cout << "  port: " << server.port << std::endl;
		// 	std::cout << "  server_name: " << server.server_name << std::endl;
		// 	std::cout << "  root: " << server.root << std::endl;
		// 	std::cout << "  client_max_body_size: " << server.client_max_body_size << std::endl;
		// 	std::cout << "  error_pages: " << server.error_pages.size() << std::endl;
		// }
	}
	catch (const std::exception &e)
	{
		std::cerr << "Config parse error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}