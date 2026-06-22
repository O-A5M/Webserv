#include "../../inc/Client.hpp"

#include "../../inc/CommoneGatewayInterface.hpp"
#include "../../inc/Response.hpp"

ClientHandler::ClientHandler(int fd, ServerConfig &config, EventLoop &loop)
		: AHandler(fd, config, loop), serverConfigs(1, config), router(serverConfigs)
{
	loop.AddHandler(this, EPOLLIN);
}

ClientHandler::~ClientHandler(void) {
    if (fd != -1)
        close(fd);
}

std::string	ClientHandler::getInterpreterPath(void) const {
	std::vector<LocationConfig>::iterator it = serverConf.locations.begin();
	std::vector<LocationConfig>::iterator itEnd = serverConf.locations.end();

	while (it != itEnd) {
		if (!it->cgi_path.empty())
			break;
		++it;
	}
	std::cout << "Interpreter path: " << it->cgi_path << std::endl;
	return (it->cgi_path);
}

void ClientHandler::OnRead(void)
{
	char buff[4096];
	ssize_t nread = recv(fd, buff, sizeof(buff), 0);

	if (nread == 0)
	{
		OnClose();
		return;
	}
	if (nread == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
	{
		std::cerr << "ClientHandler::OnRead() error: "
							<< strerror(errno) << std::endl;
		OnClose();
		return;
	}

	readBuf.append(buff, nread);
	int parse_status = this->req.parse_request(readBuf);
	if (parse_status == PARSE_WAITING)
		return;

	Response res;

	if (parse_status == PARSE_BAD_REQUEST)
	{
		res = Response::generateErrorResponse(400);
	}
	else if (parse_status == 1)
	{
		int status = this->req.validateRequest();
		if (status != OK)
			res = Response::generateErrorResponse(status);
		else
		{
			// RouteContext context = getMockRouteContext(1);
			RouteResult route_result = this->router.route(this->req, this->GetServerConf().port);
			for (size_t i = 0; i < route_result.allow_methods.size(); ++i)
			{
				std::cout << "Allowed method: " << route_result.allow_methods[i] << std::endl;
			}
			if (route_result.is_cgi) {
				std::cout << "script Path " << route_result.cgi_script_path << std::endl;
				CgiHandler::Launch(route_result.cgi_script_path
					, getInterpreterPath(), route_result.cgi_env
					,req.getBody(), serverConf, loop, *this);
			}
			// std::cout << "status=" << route_result.status
			// 					<< "max body length" << route_result.matched_location->client_max_body_size
			// 					<< ", physique_path=" << route_result.physicalPath
			// 					<< ", victore size=" << route_result.allow_methods.size()
			// 					<< ", filesystem_path=" << route_result.filesystem_path
			// 					<< ", is_cgi=" << route_result.is_cgi
			// 					<< ", is_autoindex=" << route_result.is_autoindex
			// 					<< ", is_directory=" << route_result.is_directory
			// 					<< ", is_file=" << route_result.is_file
			// 					<< ", is_redirect=" << route_result.is_redirect
			// 					<< ", redirect_location=" << route_result.redirect_location
			// 					<< ", reason=" << route_result.reason
			// 					<< std::endl;
			res.build(this->req, route_result);
		}
	}
	this->writeBuf = res.getRawResponse();
	if (!writeBuf.empty())
		EnableWrite();
	this->req.clear();
}

void ClientHandler::OnWrite(void)
{
	while (!writeBuf.empty())
	{
		ssize_t nwrite = send(fd, writeBuf.data(), writeBuf.size(), 0);
		if (nwrite == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			std::cerr << "ClientHandler: OnWrite() error: "
								<< strerror(errno) << std::endl;
			OnClose();
			return;
		}
		writeBuf.erase(0, nwrite);
	}
	writeBuf.clear();
	DisableWrite();
}

void ClientHandler::OnClose(void)
{
	loop.RemoveHandler(this);
	delete this;
}

// void ClientHandler::OnCgiResponse(const std::string &cgiRequest) {
// 	Response res;
//
// 	// TODO: res.buildFromCgiResponse(cgiRequest);
// 	this->writeBuf = res.getRawResponse();
// 	if (!writeBuf.empty())
// 		EnableWrite();
// }
