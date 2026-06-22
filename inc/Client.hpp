#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "EventLoop.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include "AHandler.hpp"
#include "Request.hpp"
#include "Router.hpp"
#include <vector>

class ClientHandler : public AHandler {
private:
    std::string         readBuf;
    std::string         writeBuf;
    std::vector<ServerConfig> serverConfigs;
    Router              router;
    Request             req;
    std::string getInterpreterPath(void) const;

public:
    ClientHandler(int fd, ServerConfig& config, EventLoop& loop);
    ~ClientHandler(void);

    void OnRead();
    void OnWrite();
    void OnClose();
    // void OnCgiResponse(const std::string &cgiResponse);
};

#endif
