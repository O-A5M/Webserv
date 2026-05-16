#ifndef SERVER_HANDLER_HPP
#define SERVER_HANDLER_HPP

#include "AHandler.hpp"
#include "ClientHandler.hpp"
#include "EventLoop.hpp"
#include <sys/socket.h>
#include "Client.hpp"

class ServerHandler : public AHandler {
public:
    ServerHandler(int listenFd, EventLoop& loop);
    ~ServerHandler(void);

    void OnRead(void);
    void OnWrite(void);
    void OnError(void);
};

#endif