#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../Event_Loop/EventLoop.hpp"
#include <unistd.h>
#include <sys/epoll.h>

class Client : public IEventLoopHandler {
private:
    int fd;

public:
    Client(const int fd);
    ~Client() override;
    int OnRead(int fd, char *buf, int len, int flags) override;
    int OnWrite(int fd, char *buf, int len, int flags) override;
    int OnError(int fd, char *buf, int len, int flags) override;
    int GetFileDescriptor(void) override;
};

#endif