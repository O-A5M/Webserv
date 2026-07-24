#ifndef AHANDLER_HPP
#define AHANDLER_HPP

#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include "EventLoop.hpp"
#include "Server.hpp"

#define TIMEOUT_SECONDS 5

class EventLoop;

class AHandler {
protected:
    int             fd;
    ServerConfig    &serverConf;
    EventLoop       &loop;
    time_t          lastActivity;
    int             timeoutSeconds;

    void    SetNonBlocking() const;
public:
    AHandler(int fd, ServerConfig &config, EventLoop& loop);
    virtual ~AHandler();

    virtual void    OnRead() = 0;
    virtual void    OnWrite() = 0;
    virtual void    OnClose() = 0;
    virtual void    OnTimeout() = 0;

    void Touch();
    bool IsTimedOut(time_t now) const;
    void SetTimeout(int seconds);

    void    EnableWrite();
    void    DisableWrite();
    int     GetFd() const;
    ServerConfig    &GetServerConf() const;
};

#endif