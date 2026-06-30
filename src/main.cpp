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
		std::cerr << "Usage: " << av[0] << " <config_file>" << std::endl;
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

	

		for (std::vector<ServerConfig>::iterator it = serverConf.begin(); it != serverConf.end(); ++it) {
			std::cout << "Server: " << it->server_name << ", Port: " << it->port << std::endl;
			std::cout << "Error Pages: " << std::endl;
			std::cout << "  Root: " << it->locations[0].upload_store << std::endl;
			for (std::map<int, std::string>::iterator epIt = it->error_pages.begin(); epIt != it->error_pages.end(); ++epIt) {
				std::cout << "  " << epIt->first << ": " << epIt->second << std::endl;
			}
		}
		servers.reserve(serverConf.size());
		for (int i = 0; i < static_cast<int>(serverConf.size()); i++) {
			servers.push_back(Server(serverConf[i]));
			servers[i].initialize_socket();
			// std::cout << "====>" << servers[i].GetFd() << std::endl;
			new ServerHandler(servers[i].GetFd(), servers[i].GetConfig(), loop);
		}
		loop.Loop();
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
		// 	// display location configs
		// 	for (std::size_t j = 0; j < server.locations.size(); ++j)
		// 	{
		// 		const LocationConfig &location = server.locations[j];
		// 		std::cout << "    location[" << j << "]" << std::endl;
		// 		std::cout << "      path: " << location.path << std::endl;
		// 		std::cout << "      root: " << location.root << std::endl;
		// 		std::cout << "      client_max_body_size: " << location.client_max_body_size << std::endl;
		// 		std::cout << "      autoindex: " << (location.autoindex ? "on" : "off") << std::endl;
		// 		std::cout << "	  allow_methods: ";
		// 		for (std::size_t k = 0; k < location.allow_methods.size(); ++k)
		// 		{
		// 			std::cout << location.allow_methods[k] << " ";
		// 		}
		// 		std::cout << std::endl;
		// 		std::cout << "      index: ";
		// 		for (std::size_t k = 0; k < location.index.size(); ++k)
		// 		{
		// 			std::cout << location.index[k] << " ";
		// 		}
		// 		std::cout << std::endl;
		// 		std::cout << "      cgi_extension: " << location.cgi_extension << std::endl;
		// 		std::cout << "      cgi_path: " << location.cgi_path << std::endl;
		// 		std::cout << "      upload_store: " << location.upload_store << std::endl;
		// 		std::cout << "      redirect: " << location.redirect << std::endl;
		// 	}
		// }
	}
	catch (const std::exception &e)
	{
		std::cerr << "Config parse error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
