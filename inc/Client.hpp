#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "EventLoop.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include "AHandler.hpp"

class ClientHandler : public AHandler {
private:
    std::string         readBuf;
    std::string         writeBuf;

public:
    ClientHandler(int fd, ServerConfig& config, EventLoop& loop);
    ~ClientHandler(void);

    void OnRead();
    void OnWrite();
    void OnError();
};

#endif