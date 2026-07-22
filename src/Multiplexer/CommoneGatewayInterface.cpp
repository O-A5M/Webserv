#include "../../inc/CommoneGatewayInterface.hpp"
#include "../../inc/Client.hpp"
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>

CgiHandler::CgiHandler(int fd,
                       int WriteFd,
                       pid_t pid,
                       ServerConfig& config,
                       EventLoop& loop,
                       ClientHandler& client,
                       const std::string& body)
    : AHandler(fd, config, loop)
    , client(&client)
    , pid(pid)
    , writeBuf(body)
    , cgiWrite(NULL) {
    uint32_t flags = EPOLLIN;
    if (!writeBuf.empty())
        cgiWrite = new CgiWriteHandler(WriteFd, config, loop, *this);
    else
        close(WriteFd);
    loop.AddHandler(this, flags);
    SetTimeout(5);
}

CgiHandler::~CgiHandler() {
    if (fd != -1)
        close(fd);
    if (cgiWrite) {
        loop.RemoveHandler(cgiWrite);
        delete cgiWrite;
        cgiWrite = NULL;
    }
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
    if (writeBuf.empty()) {
        // shutdown(fd, SHUT_WR);
        DisableWrite();
    }
}

void CgiHandler::OnWriteFd() {
    ssize_t n = write(cgiWrite->GetFd(), writeBuf.data(), writeBuf.size());
    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        OnClose();
        return;
    }
    writeBuf.erase(0, n);
    if (writeBuf.empty()) {
        loop.RemoveHandler(cgiWrite);
        delete cgiWrite;
        cgiWrite = NULL;
    }
}

CgiHandler* CgiHandler::Launch(
    const std::string&                      scriptPath,
    const std::string&                      interpreter,
    const std::map<std::string, std::string>& env,
    const std::string&                      body,
    ServerConfig&                           config,
    EventLoop&                              loop,
    ClientHandler&                          client
)
{
    int stdinPipe[2];
    int stdoutPipe[2];

    if (pipe(stdinPipe) == -1 || pipe(stdoutPipe) == -1) {
        std::cerr << "CgiHandler::Launch pipe: " << strerror(errno) << "\n";
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "CgiHandler::Launch fork: " << strerror(errno) << "\n";
        close(stdinPipe[0]);  close(stdinPipe[1]);
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        return NULL;
    }

    if (pid == 0) {
        dup2(stdinPipe[0],  STDIN_FILENO);
        dup2(stdoutPipe[1], STDOUT_FILENO);

        close(stdinPipe[0]);  close(stdinPipe[1]);
        close(stdoutPipe[0]); close(stdoutPipe[1]);

        std::vector<std::string> envStorage;
        std::vector<char*>       envp;

        envStorage.reserve(env.size());
        envp.reserve(env.size() + 1);
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

        execve(interpreter.c_str(), argv, envp.data());
        std::cerr << "CgiHandler::Launch execve: " << strerror(errno) << "\n";
        exit(1);
    }

    close(stdinPipe[0]);
    close(stdoutPipe[1]);

    return new CgiHandler(stdoutPipe[0], stdinPipe[1],
                          pid, config, loop, client, body);
}

void CgiHandler::OnClose() {
    KillChild();
    loop.RemoveHandler(this);
    if (client)
        client->ClearActiveCgi();
    // TODO: tell client to send 502
    delete this;
}

void CgiHandler::Finalize() {
    if (pid != -1) {
        int status = 0;
        waitpid(pid, &status, WNOHANG);
        pid = -1;
    }

    // std::cout << readBuf << std::endl;
    if (client) {
        client->OnCgiResponse(readBuf);
        client->ClearActiveCgi();
    }
    OnClose();
}

void CgiHandler::KillChild() {
    if (pid != -1) {
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
        pid = -1;
    }
}

CgiWriteHandler::CgiWriteHandler(int fd, ServerConfig& config, EventLoop& loop, CgiHandler& cgi)
    : AHandler(fd, config, loop)
    , cgi(cgi) {
    loop.AddHandler(this, EPOLLOUT);
}

CgiWriteHandler::~CgiWriteHandler() {}

void    CgiWriteHandler::OnRead() {}

void    CgiWriteHandler::OnWrite() {
    cgi.OnWriteFd();
}

void    CgiWriteHandler::OnClose() {
    loop.RemoveHandler(this);
    delete this;
}

void CgiHandler::OnTimeout() {
    KillChild();
    if (client) {
        client->ClearActiveCgi();
        client->OnCgiTimeout();
    }
    loop.RemoveHandler(this);
    delete this;
}

void CgiHandler::detachClient() {
    client = NULL;
}
