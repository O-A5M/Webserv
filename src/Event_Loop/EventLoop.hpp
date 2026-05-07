#ifndef EVENTLOOP_HPP
#define EVENTLOOP_HPP

class EventLoop {
private:
    int fd;

public:
    EventLoop();
    ~EventLoop();
    void    Loop();
};
#endif