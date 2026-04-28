#include "Request.hpp"

std::string raw =
		"GET /search?q=walid HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello";

void parse_request(const std::string &raw, Request &req)
{

}
