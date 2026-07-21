#include "../../inc/AHandler.hpp"

#include <ctime>

AHandler::AHandler(int fd, ServerConfig &config, EventLoop& loop)
    : fd(fd)
    , serverConf(config)
    , loop(loop)
    , lastActivity(std::time(NULL))
    , timeoutSeconds(0) {
    SetNonBlocking();
    // if (fd == -1) {
    //     std::cerr << "Webserv: invalid fd\n";
    //     throw std::runtime_error("invalid fd");
    // }
}

AHandler::~AHandler(void) {
    // if (fd != -1)
    //     close(fd);
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

void    AHandler::SetNonBlocking() const {
    const int fdflags = fcntl(fd, F_GETFD);
    const int flflags = fcntl(fd, F_GETFL);

    if (fdflags == -1 || flflags == -1) {
        std::cerr << "Webserv: fcntl(): "
                  << strerror(errno) << "\n";
        throw std::runtime_error("fcntl failed");
    }

    if (fcntl(fd, F_SETFL, flflags | O_NONBLOCK) == -1) {
        std::cerr << "Webserv: fcntl(): "
                  << strerror(errno) << "\n";
        throw std::runtime_error("fcntl failed");
    }
    if (fcntl(fd, F_SETFD, fdflags | FD_CLOEXEC) == -1) {
        std::cerr << "Webserv: fcntl(): "
                  << strerror(errno) << "\n";
        throw std::runtime_error("fcntl failed");
    }
}

void AHandler::Touch() { lastActivity = std::time(NULL); }

bool AHandler::IsTimedOut(time_t now) const {
    return timeoutSeconds > 0 && (now - lastActivity) >= timeoutSeconds;
}

void AHandler::SetTimeout(int seconds) { timeoutSeconds = seconds; }

void AHandler::OnTimeout() { OnClose(); }   // sensible default