#ifndef COMMONEGATEWAYINTERFACE_HPP
#define COMMONEGATEWAYINTERFACE_HPP

#include "AHandler.hpp"
#include "EventLoop.hpp"
#include "serverConfig.hpp"
#include <map>
#include <string>
#include <sys/wait.h>
#include <signal.h>

class ClientHandler;
class CgiWriteHandler;

class CgiHandler : public AHandler
{
private:
    ClientHandler *client;
    pid_t pid;
    std::string writeBuf;
    std::string readBuf;
    CgiWriteHandler *cgiWrite;

public:
    int WriteFd;
    CgiHandler(int fd,
               int writeFd,
               pid_t pid,
               ServerConfig &config,
               EventLoop &loop,
               ClientHandler &client,
               const std::string &body);
    ~CgiHandler();

    void OnRead();
    void OnWrite();
    void OnClose();
    void OnWriteFd();
    void OnTimeout();

    static CgiHandler *Launch(
        const std::string &scriptPath,
        const std::string &interpreter,
        const std::map<std::string, std::string> &env,
        const std::string &body,
        ServerConfig &config,
        EventLoop &loop,
        ClientHandler &client);

    void detachClient(void);

private:
    void Finalize();
    void KillChild();
};

class CgiWriteHandler : public AHandler
{
private:
    CgiHandler &cgi;

public:
    CgiWriteHandler(int fd, ServerConfig &config, EventLoop &loop, CgiHandler &cgi);
    ~CgiWriteHandler();

    void OnRead();
    void OnWrite();
    void OnClose();
    void OnTimeout() {}
};

#endif