#include "Server.hpp"
#include <sstream>

Server::Server(const std::vector<ServerConfig>& servers)
: _servers(servers)
{
}

Server::~Server()
{
}

static std::string intToString(int v) {
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

void Server::initialize_socket()
{
    for(size_t i = 0; i < _servers.size(); i++)
    {
        ServerConfig &srv = _servers[i];
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        struct addrinfo *res = NULL;
        std::string _host;
        if (!srv.host.empty())
            _host = srv.host;
        std::string portStr = intToString(srv.port);
        int status = getaddrinfo(_host.c_str(), portStr.c_str(), &hints, &res);
        if (status != 0)
            throw std::runtime_error("getaddrinfo failed!");
        for(struct addrinfo *p = res; p != NULL; p = p->ai_next)
        {
            serverFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (serverFd < 0)
                continue;
            
            int opt = 1;
            setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            if (bind(serverFd, p->ai_addr, p->ai_addrlen) < 0)
            {
                close(serverFd);
                serverFd = -1;
                continue;
            }

            if (listen(serverFd, 128) < 0)
            {
                close(serverFd);
                serverFd = -1;
                continue;
            }
            break;
        }

        freeaddrinfo(res);

        if(serverFd < 0)
            throw std::runtime_error("Failed to bind/listen");
        
        _pollfds.push_back(serverFd);
    }
}