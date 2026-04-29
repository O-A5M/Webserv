#include "Request.hpp"

std::string raw =
		"GET /search?q=walid HTTP/1.1\r\n"
		"Host: localhost\n"
		"Content-Type: text/plain\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello";

int parse_request_line(std::string req_line , Request &req)
{
	std::string method_str;
	std::string uri;
	std::string version;
	int space_count = 0;
	if (req_line[0] == ' ')
		return -1;
	for (size_t i = 0; i < req_line.size(); i++)
	{
		if (req_line[i] == ' ' && (i + 1 < req_line.size() && req_line[i + 1] == ' '))
				return -1;
		if (req_line[i] == ' '){
			space_count++;
			continue;
		}
		if (space_count == 0)
			method_str += req_line[i];
		else if (space_count == 1)
			uri += req_line[i];
		else if (space_count == 2)
			version += req_line[i];
		else
			return -1;
	}
	if (space_count != 2 || method_str.empty() || uri.empty() || version.empty())
		return -1;
	if (method_str == "GET")
		req.setMethod(GET);
	else if (method_str == "POST")
		req.setMethod(POST);
	else if (method_str == "DELETE")
		req.setMethod(DELETE);
	else
		req.setMethod(UNKNOWN);
	req.setUri(uri);
	req.setVersion(version);
	std::cout << "method: " << req.getMethod() << std::endl;
	std::cout << "uri: " << req.getUri() << std::endl;
	std::cout << "version: " << req.getVersion() << std::endl;
	return 0;
}

int parse_request_headers(std::string header, Request &req)
{

	return 0;
}

		void parse_request(const std::string &raw, Request &req)
{
	size_t pos = raw.find("\r\n\r\n");
	size_t pos_req_line = raw.find("\r\n");
	if(pos == std::string::npos)
		std::cout << "error" << std::endl;
	std::string request_line = raw.substr(0 , pos_req_line);
	std::string header = raw.substr(pos_req_line + 1, pos - (pos_req_line + 1));
	std::string body = raw.substr(pos + 4);
	std::cout << "request line : " << request_line << std::endl;
	std::cout << "------------------------ " << std::endl;
	std::cout << "header: " << header << std::endl;
	std::cout << "------------------------ " << std::endl;
	int typeOfError = parse_request_line(request_line , req);
	std::cout << "return value: " << typeOfError << std::endl;
}
int main()
{
	Request req;
	std::cout << raw.size() << std::endl;
	parse_request(raw , req);
}
