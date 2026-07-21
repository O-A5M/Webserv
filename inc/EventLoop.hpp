#ifndef EVENTLOOP_HPP
#define EVENTLOOP_HPP

#include <iostream>
#include <cstring>
#include <cerrno>
#include "AHandler.hpp"
#include <unistd.h>
#include <sys/epoll.h>
#include <set>
#include <ctime>

class AHandler;

class EventLoop {
private:
    static const int                MAX_EVENTS = 1024;
    int                             fd;
    epoll_event                     events[MAX_EVENTS];
    std::set<AHandler*>             handlers;

public:
    EventLoop()
        : fd(-1) {

        fd = epoll_create1(EPOLL_CLOEXEC);

        if (fd == -1) {
            std::cerr << "Webserv: epoll_create1: "
                      << strerror(errno) << "\n";
            throw std::runtime_error("epoll_create1 failed");
        }
    }

    ~EventLoop() {
        if (fd != -1)
            close(fd);
    }

    int GetFd() const {
        return fd;
    }

    void AddHandler(AHandler* handler, uint32_t flags);
    void ModHandler(AHandler* handler, uint32_t flags) const;
    void RemoveHandler(AHandler* handler);

    void CheckTimeouts(void);
    void Loop();
};
#endif