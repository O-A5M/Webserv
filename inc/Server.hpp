#ifndef SERVER_HPP
#define SERVER_HPP

#include <poll.h>

#include <string>
#include <vector>

#include "serverConfig.hpp"

struct Client
{
	int         fd;
	int         server_fd;
	std::string read_buffer;
	std::string write_buffer;

	Client();
	Client(int clientFd, int serverFd);
};

class Server
{
	private:
		std::vector<ServerConfig>   _configs;
		std::vector<int>            _serverFds;
		std::vector<Client>         _clients;
		std::vector<struct pollfd>  _pollFds;

	public:
		Server(const std::vector<ServerConfig> &configs);
		~Server();

		void run(); // the forever loop

	private:
		void         setupSockets();
		int          createSocket(const ServerConfig &config);
		void         acceptClient(int server_fd);
		void         handleClient(std::size_t poll_index);
		void         removeClient(std::size_t poll_index);
		bool         isServerFd(int fd) const;
		ServerConfig &getConfig(int server_fd);
};

#endif // SERVER_HPP
