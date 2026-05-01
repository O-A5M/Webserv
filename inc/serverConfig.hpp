#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "locationConfig.hpp"

struct ServerConfig
{
	std::string host;
	std::string server_name;
	std::string root;
	std::vector<std::string> index; // READ
	std::size_t client_max_body_size;
	std::map<int, std::string> error_pages; // READ
	std::vector<LocationConfig> locations;
	int port;

	ServerConfig();
};

#endif // SERVER_CONFIG_HPP
