#include "Server_Handler.hpp"

ServerHandler::ServerHandler(int fd, EventLoop &loop)
    : AHandler(fd, loop) {
    loop.AddHandler(this, EPOLLIN);
}

ServerHandler::~ServerHandler() {
    if (fd != -1)
        close (fd);
}

void    ServerHandler::OnRead() {
    struct sockaddr_in  client_addr;
    socklen_t           client_addr_len = sizeof(client_addr);

    int client_fd = accept(fd
        ,reinterpret_cast<struct sockaddr *>(&client_addr)
        , &client_addr_len);
    if (client_fd == -1) {
        std::cerr << "Accept error: " << strerror(errno) << std::endl;
        return;
    }
    try {
        new ClientHandler(client_fd, loop, client_addr, client_addr_len);
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        close (client_fd);
    }
}

void    ServerHandler::OnWrite() {}

void    ServerHandler::OnError() {
    loop.RemoveHandler(this);
    delete this;
}