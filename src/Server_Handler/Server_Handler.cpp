#include "Server_Handler.hpp"

Server_Handler::Server_Handler(int fd, EventLoop &loop)
    : AHandler(fd, loop) {
    loop.AddHandler(this, EPOLLIN);
}

Server_Handler::~Server_Handler() {}

void    ServerHandler::OnRead() {
    struct sockaddr_in  client_addr;
    socklen_t           client_addr_len = sizeof(client_addr);

    int client_fd = accept(fd
        ,reinterpret_cast<struct sockaddr *>(&clinet_addr)
        , &client_addr_len);
    if (clinet_fd == -1) {
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