#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../Event_Loop/EventLoop.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include "../Event_Handler/AHandler.hpp"

class ClientHandler : public AHandler {
private:
    std::string         readBuf;
    std::string         writeBuf;
    struct sockaddr_in  addr;
    socklen_t           addrLen;

    bool    IsRequestComplete();

public:
    ClientHandler(int clientFd, EventLoop& loop,
                  const struct sockaddr_in &addr, socklen_t addrLen);
    ~ClientHandler(void);

    void OnRead();
    void OnWrite();
    void OnError();
};

#endif