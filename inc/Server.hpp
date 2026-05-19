#ifndef SERVER_HPP
#define SERVER_HPP

#include <poll.h>
#include <string>
#include <vector>
#include <iostream>
#include <netdb.h>
// include for close
#include <unistd.h>
#include <cstring>
#include "serverConfig.hpp"


class Server {
	private:
		std::vector<ServerConfig>	_servers;  
		std::vector<int> 			_serverFds;
	public:
		Server(const std::vector<ServerConfig>& servers);
		~Server();

		void initialize_socket();
};

#endif // SERVER_HPP
