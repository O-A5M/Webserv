#ifndef SERVER_HPP
#define SERVER_HPP

#include <poll.h>
#include <string>
#include <vector>
#include <iostream>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
// include for close
#include <unistd.h>
#include <cstring>
#include "serverConfig.hpp"


class Server {
	private:
		ServerConfig	_servers;
		int 			_serverFds;

		void	SetNonBlocking() const;

	public:
		Server(ServerConfig& servers);
		~Server();
		void initialize_socket();
		int	GetFd() const;
		ServerConfig	&GetConfig();
};

#endif // SERVER_HPP
