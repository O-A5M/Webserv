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

class CgiHandler : public AHandler {
private:
    ClientHandler&  client;
    pid_t           pid;
    std::string     writeBuf;
    std::string     readBuf;

public:
    CgiHandler(int fd,
               pid_t pid,
               ServerConfig& config,
               EventLoop& loop,
               ClientHandler& client,
               const std::string& body);
    ~CgiHandler();

    void OnRead();
    void OnWrite();
    void OnClose();

    static CgiHandler* Launch(
        const std::string&                      scriptPath,
        const std::string&                      interpreter,
        const std::map<std::string, std::string>& env,
        const std::string&                      body,
        ServerConfig&                           config,
        EventLoop&                              loop,
        ClientHandler&                          client);

private:
    void Finalize();
    void KillChild();
};

#endif