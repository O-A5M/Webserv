#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "EventLoop.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include "AHandler.hpp"
#include "Request.hpp"

class ClientHandler : public AHandler {
private:
    std::string         readBuf;
    std::string         writeBuf;
    // struct sockaddr_in  addr;
    // socklen_t           addrLen;
    Request             req;


public:
    ClientHandler(int fd, ServerConfig& config, EventLoop& loop);
    ~ClientHandler(void);

    void OnRead();
    void OnWrite();
    void OnError();
};

#endif
