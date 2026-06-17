#include "../../inc/Client.hpp"
#include "../../inc/Response.hpp"

ClientHandler::ClientHandler(int fd, ServerConfig &config, EventLoop &loop)
		: AHandler(fd, config, loop), serverConfigs(1, config), router(serverConfigs)
{
	loop.AddHandler(this, EPOLLIN);
	this->state = STATE_READING_REQUEST_LINE;
}

ClientHandler::~ClientHandler(void) {
    if (fd != -1)
        close(fd);
    // delete this;
}


























void ClientHandler::OnRead(void)
{
	char buff[4096];
	ssize_t nread = recv(fd, buff, sizeof(buff), 0);

	if (nread == 0)
	{
		OnError();
		return;
	}
	if (nread == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
	{
		std::cerr << "ClientHandler::OnRead() error: " << strerror(errno) << std::endl;
		OnError();
		return;
	}

	readBuf.append(buff, nread);

	// THE STATE MACHINE LOOP
	bool keep_parsing = true;
	while (keep_parsing)
	{
		switch (this->state)
		{
		case STATE_READING_REQUEST_LINE:
		{
			size_t pos = readBuf.find("\r\n");
			if (pos == std::string::npos)
			{
				keep_parsing = false;
				break;
			}
			std::string request_line = readBuf.substr(0, pos);
			int line_status = this->req.parse_request_line(request_line);
			if (line_status < 0)
			{
				this->error_code = 400;
				this->state = STATE_ERROR;
			}
			else
			{
				readBuf.erase(0, pos + 2);
				this->state = STATE_READING_HEADERS;
			}
			break;
		}
		case STATE_READING_HEADERS:
		{
			size_t pos = readBuf.find("\r\n\r\n");
			if (pos == std::string::npos)
			{
				keep_parsing = false;
				break;
			}
			std::string header_data = readBuf.substr(0, pos + 4);
			int header_status = this->req.parse_request_headers(header_data);

			if (header_status < 0)
			{
				this->error_code = 400;
				this->state = STATE_ERROR;
			}
			else
			{
				readBuf.erase(0, pos + 4);
				this->state = STATE_HEADERS_DONE;
			}
			break;
		}
		case STATE_HEADERS_DONE:
		{
			int status = this->req.validateRequest();
			if (status != OK)
			{
				this->error_code = status;
				this->state = STATE_ERROR;
				break;
			}

			// 2. Perform Routing
			this->route_result = this->router.route(this->req, this->GetServerConf().port);

			// 3. Security Check: Client Max Body Size!
			size_t expected_size = 0;
			if (this->req.getHeaders().find("content-length") != this->req.getHeaders().end())
			{
				expected_size = strtoul(this->req.getHeaders().at("content-length").c_str(), NULL, 10);
			}

			// If it's too big, reject it BEFORE parsing the body
			if (expected_size > this->route_result.matched_location->client_max_body_size)
			{
				this->error_code = 413; // Payload Too Large
				this->state = STATE_ERROR;
				break;
			}

			// Move to body parsing
			this->state = STATE_READING_BODY;
			break;
		}

		case STATE_READING_BODY:
		{
			size_t consumed_bytes = 0;
			// Reuse your exact parse_body function!
			int body_status = this->req.parse_body(readBuf, consumed_bytes);

			if (body_status < 0)
			{
				this->error_code = 400; // Malformed body
				this->state = STATE_ERROR;
			}
			else if (body_status == 1)
			{												// 1 means PARSE_WAITING in your code
				keep_parsing = false; // Wait for the next recv() chunk!
			}
			else
			{
				readBuf.erase(0, consumed_bytes);
				this->state = STATE_COMPLETE;
			}
			break;
		}

		case STATE_COMPLETE:
		{
			std::cout << "Request Fully Parsed! Building response..." << std::endl;

			Response res;
			res.build(this->req, this->route_result);

			this->writeBuf = res.getRawResponse();
			if (!this->writeBuf.empty())
			{
				EnableWrite();
			}

			// Reset state for HTTP Keep-Alive (Pipelining)
			this->req.clear();
			this->state = STATE_READING_HEADERS;

			// If there is still data in readBuf, it belongs to the NEXT request
			keep_parsing = !readBuf.empty();
			break;
		}

		case STATE_ERROR:
		{
			std::cout << "Error encountered: " << this->error_code << std::endl;

			Response res = Response::generateErrorResponse(this->error_code);
			this->writeBuf = res.getRawResponse();

			if (!this->writeBuf.empty())
			{
				EnableWrite();
			}

			keep_parsing = false; // Stop parsing this broken request
			break;
		}
		}
	}
}

























void ClientHandler::OnRead(void)
{
	char buff[4096];
	ssize_t nread = recv(fd, buff, sizeof(buff), 0);

	if (nread == 0)
	{
		OnError();
		return;
	}
	if (nread == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
	{
		std::cerr << "ClientHandler::OnRead() error: "
							<< strerror(errno) << std::endl;
		OnError();
		return;
	}

	readBuf.append(buff, nread);
	int parse_status = this->req.parse_request(readBuf);
	std::cout << "Parse status: " << parse_status << std::endl;
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
		{
			res = Response::generateErrorResponse(status);
		}
		else
		{
			std::cout << "emmmmmmmmmmmmmmmmmm" << std::endl;
			// RouteContext context = getMockRouteContext(1);
			RouteResult route_result = this->router.route(this->req, this->GetServerConf().port);
			// for (size_t i = 0; i < route_result.allow_methods.size(); ++i)
			// {
			// 	std::cout << "Allowed method: " << route_result.allow_methods[i] << std::endl;
			// }
			std::cout << this->req.getBoundary() << std::endl;
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
			OnError();
			return;
		}
		writeBuf.erase(0, nwrite);
	}
	writeBuf.clear();
	DisableWrite();
}

void ClientHandler::OnError(void)
{
	loop.RemoveHandler(this);
	delete this;
}
