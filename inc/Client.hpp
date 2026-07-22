#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "EventLoop.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <vector>

#include "AHandler.hpp"
#include "Request.hpp"
#include "Router.hpp"
#include "CommoneGatewayInterface.hpp"

enum ParseResult
{
	STATE_READING_REQUEST_LINE,
	STATE_READING_HEADERS,
	STATE_HEADERS_DONE,
	STATE_READING_BODY,
	STATE_ERROR,
	STATE_COMPLETE,
};

class ClientHandler : public AHandler {
private:
    std::string         readBuf;
    std::string         writeBuf;
    std::vector<ServerConfig> serverConfigs;
    Router              router;
    Request             req;
	CgiHandler			*activeCgi;
	RouteResult			activeCgiRouteResult;

    std::string getInterpreterPath(void) const;

public:
    ClientHandler(int fd, ServerConfig& config, EventLoop& loop);
    ~ClientHandler(void);

	void OnRead();
    void OnWrite();
    void OnClose();
	void OnTimeout();
	void OnCgiTimeout();
    void OnCgiResponse(const std::string &cgiResponse);

	void SetActiveCgi(CgiHandler *cgi);
	void ClearActiveCgi(void);

	ParseResult state;
	int error_code;
};

#endif
