#include "../../inc/Client.hpp"
#include "../../inc/Response.hpp"


ClientHandler::ClientHandler(int fd, ServerConfig &config
    , EventLoop &loop
    , const struct sockaddr_in &addr, socklen_t addrLen)
        : AHandler(fd, config, loop)
       /* , addr(addr)
        , addrLen(addrLen) */{
		(void) addrLen; // To avoid unused parameter warning
		(void) addr; // To avoid unused parameter warning
    loop.AddHandler(this, EPOLLIN);
}



ClientHandler::~ClientHandler(void) {}

void    ClientHandler::OnRead(void) {
    char    buff[4096];
    ssize_t nread = recv(fd, buff, sizeof(buff), 0);

    if (nread == 0) {
        OnError();
        return;
    }
    if (nread == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "ClientHandler::OnRead() error: "
            << strerror(errno) << std::endl;
        OnError();
        return ;
    }
    readBuf.append(buff, nread);
    int parse_status = this->req.parse_request(readBuf);
    std::cout << "s " << parse_status << std::endl;
    if (parse_status == 0)
    {
        return ;
    }
		Response res;
		if (parse_status == -1) {
        // Status -1: BAD REQUEST (e.g., malformed headers).
        res.setStatusCode(400);
        res.setReasonPhrase("Bad Request");
        // Optional: generate a generic 400 HTML body here if you want
        res.buildRawResponse();
    }
    // } else if (parse_status == 1){
		ServerConfig &my_config = this->GetServerConf();
		res.handleRequest(this->req , my_config);
    // }
    this->writeBuf = res.getRawResponse();
    if (!writeBuf.empty())
        EnableWrite();
    this->req.clear();
}

void    ClientHandler::OnWrite(void) {
    while (!writeBuf.empty()) {
        ssize_t nwrite = send(fd, writeBuf.data(), writeBuf.size(), 0);
        if (nwrite == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            std::cerr << "ClientHandler: OnWrite() error: "
                << strerror(errno) << std::endl;
            OnError();
            return;
        }
        writeBuf.erase(0, nwrite);
    }
    writeBuf.clear();
    DisableWrite();
}

void    ClientHandler::OnError(void) {
    loop.RemoveHandler(this);
    delete this;
}
