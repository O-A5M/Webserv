#include "Request.hpp"

std::string raw =
		"GET /search?q=walid HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello";

int parse_request_line(std::string req_line)
{
	std::string buffer;
	for (size_t i = 0; i < req_line.size(); i++)
	{
		if (req_line[i] == ' ' && req_line[i + 1]  && req_line[i + 1] == ' ')
			return 1;
	}
	size_t pos = raw.find(" ");
	buffer = raw.substr(0 , pos);
	return 0;
} 
void parse_request(const std::string &raw, Request &req)
{
	size_t pos = raw.find("\r\n\r\n");
	size_t pos_req_line = raw.find("\n");
	if(pos == std::string::npos)
		std::cout << "error" << std::endl;
	std::string request_line = raw.substr(0 , pos_req_line);
	std::string header = raw.substr(pos_req_line + 1, pos - (pos_req_line + 1));
	//std::cout << "pos: " << pos << std::endl;
	std::string body = raw.substr(pos + 4);

	std::cout << "request line : " << request_line << std::endl;
	std::cout << "------------------------ " << std::endl;
	std::cout << "header: " << header << std::endl;
	std::cout << "------------------------ " << std::endl;
	std::cout << "body: " << body << std::endl;
}
int main()
{
	Request req;
	std::cout << raw.size() << std::endl;
	parse_request(raw , req);
}
