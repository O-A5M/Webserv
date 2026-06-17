#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "EventLoop.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include "AHandler.hpp"
#include "Request.hpp"
#include "Router.hpp"
#include <vector>

enum ParseResult
{
	STATE_READING_REQUEST_LINE
	STATE_READING_HEADERS,
	STATE_HEADERS_DONE,
	STATE_READING_BODY,
	PARSE_WAITING,
	PARSE_SUCCESS,
	PARSE_BAD_REQUEST = 400,		 // 400
	PARSE_HEADER_TOO_LARGE = 431 // 431
};

class ClientHandler : public AHandler {
private:
    std::string         readBuf;
    std::string         writeBuf;
    std::vector<ServerConfig> serverConfigs;
    Router              router;
    Request             req;


public:
    ClientHandler(int fd, ServerConfig& config, EventLoop& loop);
		ClientHandler();
    ~ClientHandler(void);
		ParseResult state;
		int error_code;
		void OnRead();
    void OnWrite();
    void OnError();
};

#endif
