#include "../../inc/AHandler.hpp"

// EVENT HANDLER BASE CLASS
AHandler::AHandler(int fd, ServerConfig &config, EventLoop& loop)
    : fd(fd)
    , serverConf(config)
    , loop(loop) {

    // if (fd == -1) {
    //     std::cerr << "Webserv: invalid fd\n";
    //     throw std::runtime_error("invalid fd");
    // }
}

AHandler::~AHandler(void) {
    if (fd != -1)
        close(fd);
}

void    AHandler::EnableWrite(void) {
    loop.ModHandler(this, EPOLLIN | EPOLLOUT);
}

void    AHandler::DisableWrite(void) {
    loop.ModHandler(this, EPOLLIN);
};

int AHandler::GetFd(void) const {
    return fd;
}

ServerConfig    &AHandler::GetServerConf(void) const {
    return serverConf;
}

// void    AHandler::SetNonBlocking() const {
//     const int flags = fcntl(fd, F_GETFL);
//
//     if (flags == -1) {
//         std::cerr << "Webserv: fcntl(F_GETFL): "
//                   << strerror(errno) << "\n";
//         throw std::runtime_error("fcntl failed");
//     }
//
//     if (fcntl(fd, F_SETFL, flags | O_NONBLOCK | FD_CLOEXEC) == -1) {
//         std::cerr << "Webserv: fcntl(F_SETFL): "
//                   << strerror(errno) << "\n";
//         throw std::runtime_error("fcntl failed");
//     }
// }