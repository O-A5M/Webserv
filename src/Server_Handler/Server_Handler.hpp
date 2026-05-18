#ifndef SERVER_HANDLER_HPP
#define SERVER_HANDLER_HPP

#include "../Event_Handler/AHandler.hpp"
#include "../Client/Client.hpp"
#include "../Event_Loop/EventLoop.hpp"
#include <sys/socket.h>
#include <netinet/in.h>

class ServerHandler : public AHandler {
public:
    ServerHandler(int listenFd, EventLoop& loop);
    ~ServerHandler(void);

    void OnRead(void);
    void OnWrite(void);
    void OnError(void);
};

#endif