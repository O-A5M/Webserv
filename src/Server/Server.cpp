#include "Server.hpp"
#include <sstream>
#include <map>
#include <set>

Server::Server(const std::vector<ServerConfig>& servers)
: _servers(servers)
{
}

Server::~Server()
{
    std::set<int> closed;
    for (size_t i = 0; i < _serverFds.size(); ++i)
    {
        const int fd = _serverFds[i];
        if (fd >= 0 && closed.insert(fd).second)
            close(fd);
    }
}

static std::string intToString(int v) {
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

void Server::initialize_socket()
{
	std::map<int, int> portToFd;

    for(size_t i = 0; i < _servers.size(); i++)
    {
        ServerConfig &srv = _servers[i];

        // std::map<int, int>::const_iterator it = portToFd.find(srv.port);
        // if (it != portToFd.end())
        // {
        //     _serverFds.push_back(it->second);
        //     continue;
        // }

        int fd = -1;

        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET; // IPv4
        hints.ai_socktype = SOCK_STREAM; // TCP
        hints.ai_flags = AI_PASSIVE; // Use my IP

        struct addrinfo *res = NULL;

        const char *_host;
        if (!srv.host.empty())
            _host = srv.host.c_str();
        else
            _host = NULL;

        std::string portStr = intToString(srv.port);

        int status = getaddrinfo(_host, portStr.c_str(), &hints, &res);
        if (status != 0)
            throw std::runtime_error("getaddrinfo failed!");

        for(struct addrinfo *p = res; p != NULL; p = p->ai_next)
        {
            fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (fd < 0)
            {
                continue;
            }
            
            // int opt = 1;
            // setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            if (bind(fd, p->ai_addr, p->ai_addrlen) < 0)
            {
                close(fd);
                fd = -1;
                continue;
            }

            if (listen(fd, 128) < 0)
            {
                close(fd);
                fd = -1;
                continue;
            }
            break;
        }

        freeaddrinfo(res);

        if(fd < 0)
            throw std::runtime_error("Failed to bind/listen");

        portToFd[srv.port] = fd;
        _serverFds.push_back(fd);
    }
    for (size_t i = 0; i < _serverFds.size(); ++i)
    {
        std::cout << "Server listening on port " << _servers[i].port << " with fd " << _serverFds[i] << std::endl;
    }
}