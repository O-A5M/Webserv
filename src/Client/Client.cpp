#include "Client.hpp"

Client::Client(const int fd)
    : fd(fd) {

}

Client::~Client() {
    close(fd);
}

int Client::GetFileDescriptor() {
    return fd;
}

int Client::OnRead(int fd, char *buf, int len, int flags) {

}
