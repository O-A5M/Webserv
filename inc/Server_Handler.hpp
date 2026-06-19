#ifndef SERVER_HANDLER_HPP
#define SERVER_HANDLER_HPP

#include "AHandler.hpp"
#include "Client.hpp"
#include "EventLoop.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include "Server.hpp"
#include "serverConfig.hpp"

class ServerHandler : public AHandler {
public:
    ServerHandler(int fd, ServerConfig &config, EventLoop& loop);
    ~ServerHandler(void);

    void OnRead(void);
    void OnWrite(void);
    void Onclose(void);
};

#endif