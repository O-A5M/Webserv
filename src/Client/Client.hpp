#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../Event_Loop/EventLoop.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "../Event_Handler/AHandler.hpp"

class ClientHandler : public AHandler {
public:
    ClientHandler(const int fd, const EventLoop& loop)
        : AHandler(fd, loop) {
    }

    void OnRead() const override {
        struct sockaddr addr;
        socklen_t       addrlen = sizeof(addr);
        epoll_event     ev;
        int             clfd = -1;

        ev.events = EPOLLIN;
        ev.data.fd = fd;

        int epfd = epoll_create1(EPOLL_CLOEXEC);
        if (epfd == -1) {
            std::cerr << "epoll_create1 failed: " << std::strerror(errno) << std::endl;
            // TODO
        }

        clfd = accept(fd, &addr, &addrlen);
        if (clfd == -1) {
            std::cerr << "accept failed: " << strerror(errno) << std::endl;
            // TODO
        }

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
            std::cerr << "epoll_ctl failed: " << strerror(errno) << std::endl;
            // TODO
        }
    }
};

#endif