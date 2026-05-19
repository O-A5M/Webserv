#ifndef AHANDLER_HPP
#define AHANDLER_HPP

#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include "EventLoop.hpp"

class EventLoop;

class AHandler {
protected:
    int         fd;
    EventLoop&  loop;

private:
    void    SetNonBlocking() const;

public:
    AHandler(const int fd, EventLoop& loop);
    virtual ~AHandler();

    virtual void    OnRead() = 0;
    virtual void    OnWrite() = 0;
    virtual void    OnError() = 0;

    void    EnableWrite();
    void    DisableWrite();
    int     GetFd() const;
};

#endif