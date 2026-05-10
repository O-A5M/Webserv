#ifndef AHANDLER_HPP
#define AHANDLER_HPP
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>

#include "EventLoop.hpp"

class AHandler {
protected:
    int         fd;
    EventLoop   loop;

public:
    AHandler(const int fd, const EventLoop& loop)
        : fd(fd)
        , loop(loop) {
        if (fd == -1) {
            std::cerr << "Webserv: " << strerror(errno) << "\n";
            // TODO
        }
        if (fcntl(fd, F_SETFD, O_NONBLOCK) == -1) {
            std::cerr << "Webserv: " << strerror(errno) << "\n";
            // TODO
        }
        if (epoll_create1(EPOLL_CLOEXEC) == -1) {
            std::cerr << "Webserv: " << strerror(errno) << "\n";
            // TODO
        }
    }
    ~AHandler() {
        close(fd);
        // TODO
    }

    virtual void    OnWrite() const = 0;
    virtual void    OnRead() const = 0;
    virtual void    OnError() const = 0;

    void    EnableWrite() const {
        epoll_event ev;
        ev.events = EPOLLIN | EPOLLOUT;
        ev.data.fd = fd;

        if (epoll_ctl(fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
            std::cerr << "Webserv: " << strerror(errno) << "\n";
            // TODO
        }
    };
    void    DisableWrite() const {
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;

        if (epoll_ctl(fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
            std::cerr << "Webserv: " << strerror(errno) << "\n";
            // TODO
        }
    }

    int     GetFd() const {
        return fd;
    }
};

#endif