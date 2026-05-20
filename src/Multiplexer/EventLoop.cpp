#include "../../inc/EventLoop.hpp"

// EVENT LOOP 
void EventLoop::AddHandler(AHandler* handler, uint32_t flags) const {
    epoll_event ev;

    ev.events = flags;
    ev.data.ptr = handler;

    if (epoll_ctl(fd,
                  EPOLL_CTL_ADD,
                  handler->GetFd(),
                  &ev) == -1) {

        std::cerr << "Webserv: epoll_ctl ADD: "
                  << strerror(errno) << "\n";

        throw std::runtime_error("epoll_ctl add failed");
    }
}

void EventLoop::ModHandler(AHandler* handler, uint32_t flags) const {
    epoll_event ev;

    ev.events = flags;
    ev.data.ptr = handler;

    if (epoll_ctl(fd,
                  EPOLL_CTL_MOD,
                  handler->GetFd(),
                  &ev) == -1) {

        std::cerr << "Webserv: epoll_ctl MOD: "
                  << strerror(errno) << "\n";

        throw std::runtime_error("epoll_ctl mod failed");
    }
}

void EventLoop::RemoveHandler(AHandler* handler) const {
    if (epoll_ctl(fd,
                  EPOLL_CTL_DEL,
                  handler->GetFd(),
                  NULL) == -1) {

        std::cerr << "Webserv: epoll_ctl DEL: "
                  << strerror(errno) << "\n";
    }
}

void EventLoop::Loop() {
    int ready = 0;
    while (true) {
        ready = epoll_wait(fd,
                               events,
                               MAX_EVENTS,
                               -1);

        if (ready == -1) {
            if (errno == EINTR)
                continue;
            std::cerr << "Webserv: epoll_wait: "
                      << strerror(errno) << "\n";
            continue;
            // TODO: handle the error cleanly without stoping the server
        }

        for (int i = 0; i < ready; ++i) {
            AHandler* handler = static_cast<AHandler*>(events[i].data.ptr);
            uint32_t ev = events[i].events;

            if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                handler->OnError();
                continue;
            }
            if (ev & EPOLLIN)
                handler->OnRead();

            else if (ev & EPOLLOUT)
                handler->OnWrite();
        }
    }
}