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
    struct sockaddr_in  addr;
    socklen_t           addrLen;

public:
    ClientHandler(int clientFd, EventLoop& loop,
                  const struct sockaddr_in &addr, socklen_t addrLen);
    ~ClientHandler(void);

    void OnRead();
    void OnWrite();
    void OnError();
};

#endif