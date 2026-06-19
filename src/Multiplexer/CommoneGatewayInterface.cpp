#include "../../inc/CommoneGatewayInterface.hpp"
#include "../../inc/Client.hpp"
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>

CgiHandler::CgiHandler(int fd,
                       pid_t pid,
                       ServerConfig& config,
                       EventLoop& loop,
                       ClientHandler& client,
                       const std::string& body)
    : AHandler(fd, config, loop)
    , client(client)
    , pid(pid)
    , writeBuf(body) {
    uint32_t flags = EPOLLIN;
    if (!writeBuf.empty())
        flags |= EPOLLOUT;
    loop.AddHandler(this, flags);
}

CgiHandler::~CgiHandler() {
    if (fd != -1)
        close(fd);
}

void CgiHandler::OnRead() {
    char    buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf));

    if (n > 0) {
        readBuf.append(buf, n);
        return;
    }
    if (n == 0 || (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK))
        Finalize();
}

void CgiHandler::OnWrite() {
    ssize_t n = write(fd, writeBuf.data(), writeBuf.size());
    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        std::cerr << "CgiHandler::OnWrite: " << strerror(errno) << "\n";
        OnClose();
        return;
    }
    writeBuf.erase(0, n);
    DisableWrite();
}

void CgiHandler::OnClose() {
    KillChild();
    loop.RemoveHandler(this);
    // TODO: tell client to send 502
    delete this;
}

void CgiHandler::Finalize() {
    if (pid != -1) {
        int status = 0;
        waitpid(pid, &status, WNOHANG);
        pid = -1;
    }

    loop.RemoveHandler(this);
    std::cout << readBuf << std::endl;
    // client.OnCgiResponse(readBuf);
    delete this;
}

void CgiHandler::KillChild() {
    if (pid != -1) {
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
        pid = -1;
    }
}

CgiHandler* CgiHandler::Launch(
    const std::string&                      scriptPath,
    const std::string&                      interpreter,
    const std::map<std::string, std::string>& env,
    const std::string&                      body,
    ServerConfig&                           config,
    EventLoop&                              loop,
    ClientHandler&                          client) {

    int sockPair[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockPair) == -1) {
        std::cerr << "CgiHandler::Launch socketpair: " << strerror(errno) << "\n";
        // TODO: server error 500
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "CgiHandler::Launch fork: " << strerror(errno) << "\n";
        close(sockPair[0]);  close(sockPair[1]);
        // TODO: server error 500
        return NULL;
    }

    if (pid == 0) {
        dup2(sockPair[0],  STDIN_FILENO);
        dup2(sockPair[0], STDOUT_FILENO);

        close(sockPair[0]);
        close(sockPair[1]);

        std::string dir = scriptPath.substr(0, scriptPath.rfind('/'));
        if (!dir.empty())
            chdir(dir.c_str());

        std::vector<std::string> envStorage;
        std::vector<char*>       envp;
        for (std::map<std::string,std::string>::const_iterator it = env.begin();
             it != env.end(); ++it) {
            envStorage.push_back(it->first + "=" + it->second);
            envp.push_back(const_cast<char*>(envStorage.back().c_str()));
        }
        envp.push_back(NULL);

        char* argv[] = {
            const_cast<char*>(interpreter.c_str()),
            const_cast<char*>(scriptPath.c_str()),
            NULL
        };

        execve(interpreter.c_str(), argv, &envp[0]);
        std::cerr << "CgiHandler::Launch execve: " << strerror(errno) << "\n";
        exit(1);
    }

    close(sockPair[0]);

    return new CgiHandler(sockPair[1], pid, config,
                          loop, client, body);
}