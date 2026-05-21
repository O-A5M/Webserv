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

class EventLoop;

class AHandler
{
protected:
    int fd;
    ServerConfig &serverConf;
    EventLoop &loop;
    void SetNonBlocking() const;

public:
    AHandler(int fd, ServerConfig &config, EventLoop &loop);
    virtual ~AHandler();

    virtual void OnRead() = 0;
    virtual void OnWrite() = 0;
    virtual void OnError() = 0;

    void EnableWrite();
    void DisableWrite();
    int GetFd() const;
    ServerConfig &GetServerConf() const;
};

#endif