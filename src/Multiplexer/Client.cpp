#include "../../inc/Client.hpp"
#include "../../inc/Response.hpp"


ClientHandler::ClientHandler(int fd, ServerConfig &config
    , EventLoop &loop)
        : AHandler(fd, config, loop) {
    loop.AddHandler(this, EPOLLIN);
}

RouteContext getMockRouteContext(int test_scenario)
{
	RouteContext mock;

	if (test_scenario == 1)
	{
		// TEST 1: A perfect static file (e.g., index.html)
		mock.status = 200;
		mock.filesystem_path = "./www/index.html"; // Make sure this file actually exists on your PC!
		mock.is_file = true;
		mock.allow_methods.push_back("GET");
		mock.allow_methods.push_back("POST");
	}
	else if (test_scenario == 2)
	{
		// TEST 2: Triggering the Autoindex listing
		mock.status = 200;
		mock.filesystem_path = "www/html/";
		mock.is_autoindex = true;
		mock.allow_methods.push_back("GET");
		mock.allow_methods.push_back("POST");
	}
	else if (test_scenario == 3)
	{
		// TEST 3: Forcing a 405 Error (Like someone tried to DELETE)
		mock.status = 405;
		mock.reason = "DELETE not allowed on location /protected";
		mock.allow_methods.push_back("GET");
		mock.allow_methods.push_back("POST");
	}

	// NOTE: We leave matched_location and matched_server as NULL for now.
	// Your Response class shouldn't even need them because the booleans do all the work!
	return mock;
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
				RouteContext mog = getMockRouteContext(1);
				res.build(this->req, mog);
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
