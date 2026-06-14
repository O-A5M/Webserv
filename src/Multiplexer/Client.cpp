#include "../../inc/Client.hpp"
#include "../../inc/Response.hpp"


ClientHandler::ClientHandler(int fd, ServerConfig &config
    , EventLoop &loop)
        : AHandler(fd, config, loop)
        , serverConfigs(1, config)
        , router(serverConfigs) {
    loop.AddHandler(this, EPOLLIN);
}

ClientHandler::~ClientHandler(void) {
    if (fd != -1)
        close(fd);
    delete this;
}

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
    if (parse_status == PARSE_WAITING)
        return ;

    Response res;

    if (parse_status == PARSE_BAD_REQUEST) {
        res = Response::generateErrorResponse(400);
    }
    else if (parse_status == 1)
    {
        int status = this->req.validateRequest();
        if (status != OK)
            res = Response::generateErrorResponse(status);
        else
        {
				// RouteContext mog = getMockRouteContext(1);
            RouteResult route_result = this->router.route(this->req, this->GetServerConf().port);
				// std::cout << "RouteResult: status=" << route_result.status
                //     << ", filesystem_path=" << route_result.filesystem_path
                //     << ", is_cgi=" << route_result.is_cgi
                //     << ", is_autoindex=" << route_result.is_autoindex
                //     << ", is_directory=" << route_result.is_directory
                //     << ", is_file=" << route_result.is_file
                //     << ", is_redirect=" << route_result.is_redirect
                //     << ", redirect_location=" << route_result.redirect_location
                //     << ", reason=" << route_result.reason
                //     << std::endl;
            res.build(this->req, route_result);
        }
    }
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
