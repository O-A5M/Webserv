#ifndef AHANDLER_HPP
#define AHANDLER_HPP
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include "EventLoop.hpp"

class AHandler {
protected:
    const int   fd;
    EventLoop&  loop;

private:
    void    SetNonBlocking() const {
        const int flags = fcntl(fd, F_GETFL);

        if (flags == -1) {
            std::cerr << "Webserv: fcntl(F_GETFL): "
                      << strerror(errno) << "\n";
            throw std::runtime_error("fcntl failed");
        }

        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK | FD_CLOEXEC) == -1) {
            std::cerr << "Webserv: fcntl(F_SETFL): "
                      << strerror(errno) << "\n";
            throw std::runtime_error("fcntl failed");
        }
    }

public:
    AHandler(const int fd, EventLoop& loop)
        : fd(fd)
        , loop(loop) {

        if (fd == -1) {
            std::cerr << "Webserv: invalid fd\n";
            throw std::runtime_error("invalid fd");
        }

        SetNonBlocking();
    }

    virtual ~AHandler() {
        if (fd != -1)
            close(fd);
    }

    virtual void    OnRead() = 0;
    virtual void    OnWrite() = 0;
    virtual void    OnError() = 0;

    void    EnableWrite() {
        loop.ModHandler(this, EPOLLIN | EPOLLOUT);
    };
    void    DisableWrite() {
        loop.ModHandler(this, EPOLLIN);
    };

    int     GetFd() const {
        return fd;
    }
};

#endif