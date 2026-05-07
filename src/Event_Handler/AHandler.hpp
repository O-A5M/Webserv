#ifndef AHANDLER_HPP
#define AHANDLER_HPP
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "EventLoop.hpp"

class AHandler {
protected:
    int         fd;
    EventLoop   loop;

public:
    AHandler(int fd, EventLoop& loop)
        : fd(fd)
        , loop(loop) {
        if (fd == -1) {
            // TODO
        }
        if (fcntl(fd, F_SETFD, O_NONBLOCK) == -1) {
            // TODO
        }
    }
    ~AHandler() {
        close(fd);
    }

    virtual void    OnWrite() = 0;
    virtual void    OnRead() = 0;
    virtual void    OnError() = 0;

    void    EnableWrite() {
        // TODO
        return;
    };
    void    DisableWrite() const {
        // TODO
        return ;
    }

    int     GetFd() const {
        return fd;
    }
};

#endif