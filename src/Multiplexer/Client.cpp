#include "../../inc/Client.hpp"

#include "../../inc/CommoneGatewayInterface.hpp"
#include "../../inc/Response.hpp"

ClientHandler::ClientHandler(int fd, ServerConfig &config, EventLoop &loop)
		: AHandler(fd, config, loop), serverConfigs(1, config), router(serverConfigs)
{
	loop.AddHandler(this, EPOLLIN);
	this->state = STATE_READING_REQUEST_LINE;
	this->error_code = 0;
	SetTimeout(20);
}

ClientHandler::~ClientHandler(void)
{
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

	// THE STATE MACHINE LOOP
	bool keep_parsing = true;
	while (keep_parsing)
	{
		switch (this->state)
		{
		case STATE_READING_REQUEST_LINE:
		{
			while (readBuf.compare(0, 2, "\r\n") == 0)
				readBuf.erase(0, 2);
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

			this->req.route_result = this->router.route(this->req, this->GetServerConf().port);
			std::cout << "path" << this->req.route_result.filesystem_path << std::endl;
			if (this->req.route_result.matched_location == NULL)
			{
				this->error_code = this->req.route_result.status;
				if (this->error_code == 0)
					this->error_code = 500;
				this->state = STATE_ERROR;
				break;
			}

			size_t expected_size = 0;
			if (this->req.getHeaders().find("content-length") != this->req.getHeaders().end())
			{
				expected_size = strtoul(this->req.getHeaders().at("content-length").c_str(), NULL, 10);
			}
			if (expected_size > this->req.route_result.max_body_size)
			{
				this->error_code = 413;
				this->state = STATE_ERROR;
				break;
			}
			this->state = STATE_READING_BODY;
			break;
		}
		case STATE_READING_BODY:
		{
			size_t consumed_bytes = 0;

			int body_status = this->req.parse_body(readBuf, consumed_bytes);
			if (body_status < 0)
			{
				this->error_code = 400;
				this->state = STATE_ERROR;
			}
			else if (body_status == 1)
			{
				keep_parsing = false;
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
			// std::cout << "Request Fully Parsed! Building response..." << std::endl;
			if (this->req.route_result.is_cgi)
			{
				// Body was streamed to disk during parse_body(), not kept in req.body,
				// so read it back before handing it to the CGI process.
				std::string cgiBody;
				if (!this->req.getBodyFilePath().empty())
				{
					std::ifstream bodyFile(this->req.getBodyFilePath().c_str(), std::ios::binary);
					std::stringstream ss;
					ss << bodyFile.rdbuf();
					cgiBody = ss.str();
				}

				CgiHandler::Launch(
					this->req.route_result.cgi_script_path,
					this->req.route_result.matched_location->cgi_path,   // interpreter
					this->req.route_result.cgi_env,
					cgiBody,
					this->GetServerConf(),
					this->loop,
					*this
				);

				this->req.clear();
				this->state = STATE_READING_REQUEST_LINE;
				keep_parsing = !readBuf.empty();
				break;   // <-- don't fall through to the synchronous response build below
			}

			this->req.display();
			Response res;
			res.build(this->req, this->req.route_result);

			this->writeBuf = res.getRawResponse();
			if (!this->writeBuf.empty())
			{
				EnableWrite();
			}
			this->req.clear();
			this->state = STATE_READING_REQUEST_LINE;
			keep_parsing = !readBuf.empty();
			break;
		}
		case STATE_ERROR:
		{
			std::cout << "Error encountered: " << this->error_code << std::endl;

			Response res = Response::generateErrorResponse(this->error_code , this->req.route_result);
			this->writeBuf = res.getRawResponse();
			if (!this->writeBuf.empty())
			{
				EnableWrite();
			}
			keep_parsing = false;
			break;
		}
		}
	}
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

void ClientHandler::OnTimeout() {
	if (state != STATE_READING_REQUEST_LINE && req.route_result.matched_server != NULL) {
		Response res = Response::generateErrorResponse(408, req.route_result);
		writeBuf = res.getRawResponse();
		send(fd, writeBuf.data(), writeBuf.size(), MSG_NOSIGNAL);
	}
	OnClose();
}

void ClientHandler::OnCgiTimeout() {
	Response res = Response::generateErrorResponse(504, req.route_result);
	writeBuf = res.getRawResponse();
	if (!writeBuf.empty())
		EnableWrite();
}