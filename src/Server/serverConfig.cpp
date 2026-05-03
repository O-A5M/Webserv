#include "serverConfig.hpp"

ServerConfig::ServerConfig()
    : host("0.0.0.0"),   // listen on all interfaces by default
	server_name(""),
	root(""),
	client_max_body_size(1000000),
	port(80) // default HTTP port
{
}