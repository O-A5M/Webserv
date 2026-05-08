#include "Server.hpp"

Server::Server(const std::vector<ServerConfig>& servers)
: _servers(servers)
{
}

Server::~Server()
{
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
        // int statu = getaddrinfo(_host, srv.port, &hints, &res);
        int status = getaddrinfo(_host.c_str(), std::to_string(srv.port).c_str(), &hints, &res);
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
        // print pollfd info
    }
}