#include "Client.hpp"

ClientHandler::ClientHandler(int fd, ServerConfig &config
    , EventLoop &loop)
        : AHandler(fd, config, loop) {
    loop.AddHandler(this, EPOLLIN);
}

ClientHandler::~ClientHandler(void) {}

void    ClientHandler::OnRead(void) {
    char    buff[4096];
    ssize_t nread = recv(fd, buff, sizeof(buff), 0);

    if (nread == 0) {
        OnError();
        return;
    }
    if (nread == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "ClientHandler::OnRead() error: "
            << strerror(errno) << std::endl;
        OnError();
        return ;
    }
    readBuf = buff;
    std::cout << "ClientHandler::OnRead() " << readBuf << std::endl;
    // TODO: check if the request is complete
    // TODO: parse readBuf and build a response in writeBuf
    readBuf.clear();
    if (!writeBuf.empty())
        EnableWrite();
}

void    ClientHandler::OnWrite(void) {
    while (!writeBuf.empty()) {
        ssize_t nwrite = send(fd, writeBuf.data(), writeBuf.size(), 0);
        if (nwrite == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            std::cerr << "ClientHandler: OnWrite() error: "
                << strerror(errno) << std::endl;
            OnError();
            return;
        }
        writeBuf.erase(0, nwrite);
    }
    writeBuf.clear();
    DisableWrite();
}

void    ClientHandler::OnError(void) {
    loop.RemoveHandler(this);
    delete this;
}