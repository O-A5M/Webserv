#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
	: host("127.0.0.1"),
	  server_name("localhost"),
	  root("/var/www/html"),
	  client_max_body_size(1000000),
	  port(8080)
{
}
