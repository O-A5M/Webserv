#include "../../inc/EventLoop.hpp"

volatile sig_atomic_t EventLoop::running = 1;

void EventLoop::AddHandler(AHandler* handler, uint32_t flags) {
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
    handlers.insert(handler);
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

void EventLoop::RemoveHandler(AHandler* handler) {
    if (epoll_ctl(fd,
                  EPOLL_CTL_DEL,
                  handler->GetFd(),
                  NULL) == -1) {

        std::cerr << "Webserv: epoll_ctl DEL: "
                  << strerror(errno) << "\n";
    }
    handlers.erase(handler);
}

void EventLoop::CheckTimeouts() {
    time_t now = std::time(NULL);
    std::vector<AHandler*> expired;

    for (std::set<AHandler*>::iterator it = handlers.begin(); it != handlers.end(); ++it) {
        if ((*it)->IsTimedOut(now))
            expired.push_back(*it);
    }

    for (size_t i = 0; i < expired.size(); ++i)
        expired[i]->OnTimeout();
}

void EventLoop::Loop() {
    int ready = 0;
    while (running) {
        ready = epoll_wait(fd,
                               events,
                               MAX_EVENTS,
                               1000);

        if (ready == -1) {
            continue;
        }

        for (int i = 0; i < ready; ++i) {
            AHandler* handler = static_cast<AHandler*>(events[i].data.ptr);
            uint32_t ev = events[i].events;

            if (ev & EPOLLERR) {
                handler->OnClose();
                continue;
            }
            if (ev & (EPOLLIN | EPOLLHUP | EPOLLRDHUP))
                handler->OnRead();

            else if (ev & EPOLLOUT)
                handler->OnWrite();
        }
        CheckTimeouts();
    }
}

void EventLoop::shutdown() {
    while (!handlers.empty()) {
        AHandler* handler = *handlers.begin();
        handler->OnClose();
    }
}
