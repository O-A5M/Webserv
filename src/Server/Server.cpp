#include "Server.hpp"
#include <sstream>
#include <map>
#include <set>

Server::Server(ServerConfig& servers)
: _servers(servers)
, serverConfigs(1, servers)
, router(serverConfigs)
, _serverFds(-1)
{
}

Server::~Server()
{
    // if (_serverFds != 1) {
    //     close (_serverFds);
    //     _serverFds = -1;
    // }
}

void    Server::SetNonBlocking() const {
    const int fdflags = fcntl(_serverFds, F_GETFD);
    const int flflags = fcntl(_serverFds, F_GETFL);

    if (fdflags == -1 || flflags == -1) {
        std::cerr << "Webserv: fcntl(): "
                  << strerror(errno) << "\n";
        throw std::runtime_error("fcntl failed");
    }

    if (fcntl(_serverFds, F_SETFL, flflags | O_NONBLOCK) == -1) {
        std::cerr << "Webserv: fcntl(): "
                  << strerror(errno) << "\n";
        throw std::runtime_error("fcntl failed");
    }
    if (fcntl(_serverFds, F_SETFD, fdflags | FD_CLOEXEC) == -1) {
        std::cerr << "Webserv: fcntl(): "
                  << strerror(errno) << "\n";
        throw std::runtime_error("fcntl failed");
    }
}

static std::string intToString(int v) {
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

void Server::initialize_socket()
{
    int fd = -1;
    ServerConfig &srv = _servers;
    struct addrinfo hints = {};
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // Use my IP

    struct addrinfo *res = NULL;

    const char *host;
    if (!srv.host.empty())
        host = srv.host.c_str();
    else
        host = NULL;

    std::string portStr = intToString(srv.port);

    int status = getaddrinfo(host, portStr.c_str(), &hints, &res);
    if (status != 0)
        throw std::runtime_error("getaddrinfo failed!");

    for (struct addrinfo *p = res; p != NULL; p = p->ai_next)
    {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0)
            continue;

        int opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            close(fd);
            fd = -1;
            continue;
        }

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

    _serverFds = fd;
    SetNonBlocking();

    std::cout << "Server listening on port " << _servers.port << " with fd " << _serverFds << std::endl;
}

int Server::GetFd() const {
    return _serverFds;
}

ServerConfig    &Server::GetConfig() {
    return _servers;
}