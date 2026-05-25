#include "serverConfig.hpp"

ServerConfig::ServerConfig()
    : server_name(""),
	root(""),
	client_max_body_size(1000000),
	port(80) // default HTTP port
{
}