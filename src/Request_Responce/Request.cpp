#include "../../inc/Request.hpp"
#include "../../inc/Response.hpp"

std::string raw =
		"GET /index.css HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Transfer-Encoding: chunked\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"5\r\n"
		"Hello\r\n"
		"a\r\n"
		", webserv!\r\n"
		"0\r\n"
		"\r\n";

int Request::parse_request_line(const std::string &req_line)
{
	std::string method_str;
	std::string uri;
	std::string version;
	int space_count = 0;
	if (req_line[0] == ' ')
		return -1;
	for (size_t i = 0; i < req_line.size(); i++)
	{
		if (req_line[i] == '\r')
			continue;
		if (req_line[i] == ' ' && (i + 1 < req_line.size() && req_line[i + 1] == ' '))
			return -1;
		if (req_line[i] == ' ')
		{
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
		this->setMethod(GET);
	else if (method_str == "POST")
		this->setMethod(POST);
	else if (method_str == "DELETE")
		this->setMethod(DELETE);
	else
		this->setMethod(UNKNOWN);
	this->setUri(uri);
	size_t pos = uri.find("?");
	if (pos != std::string::npos)
		this->setQuery(uri.substr(pos + 1, uri.size() - pos - 1));
	this->setPath(uri.substr(0, pos));
	this->setVersion(version);
	return 0;
}

int Request::parse_request_headers_helper(const std::string &header, size_t startIndex)
{
	for (size_t i = startIndex; i < header.size(); i++)
	{
		if (header[i] == '\r' || header[i] == '\n')
			return 0;
		if (header[i] != ' ' && header[i] != '\t')
			return 1;
	}
	return 0;
}

void Request::skip_whitespace(const std::string &header, size_t &i)
{
	while (i < header.size() && (header[i] == ' ' || header[i] == '\t'))
		i++;
}

int Request::parse_request_headers(const std::string &header)
{
	int flag = 0;
	int errorR = 0;
	int colonFlag = 0;
	for (size_t i = 0; i < header.size(); i++)
	{
		if (header[i] == '\r')
		{
			flag = 1;
			continue;
		}
		if (flag)
		{
			if (header[i] != '\n')
				return -1;
			flag = 0;
			continue;
		}
		if (flag == 0)
		{
			std::string key;
			while (i < header.size() && header[i] != ':' && header[i] != '\n')
			{
				if (header[i] == ' ' || header[i] == '\t')
					return -2;
				key += std::tolower(header[i++]);
			}
			if (header[i] != ':')
				return -1;
			i++;
			std::string value;

			skip_whitespace(header, i);
			while (i < header.size() && header[i] != '\n')
			{
				if (header[i] == ' ' || header[i] == '\t')
				{
					if (parse_request_headers_helper(header, i) == 0)
					{
						i++;
						continue;
					}
				}
				if (header[i] == '\r')
				{
					i++;
					continue;
				}
				value += header[i++];
			}
			errorR = this->setHeader(key, value);
			if (errorR < 0)
			{
				if (errorR == -2)
					colonFlag = 1;
				else
					return -1;
			}
		}
	}
	if (this->getHeaders().find("host") == this->getHeaders().end())
	{
		std::cout << "Host header does not exist" << std::endl;
		return -3;
	}
	if ((this->getHeaders().find("transfer-encoding") != this->getHeaders().end()))
	{
		if (this->getHeaders().find("content-length") != this->getHeaders().end())
		{
			this->removeHeader("content-length");
		}
	}
	else if ((this->getHeaders().find("content-length") != this->getHeaders().end()) && colonFlag == 0)
	{
		char *end;
		long n = strtol(this->getHeaders().find("content-length")->second.c_str(), &end, 10);
		if (*end != '\0' || n < 0)
			return -1;
	}
	else if (colonFlag)
	{
		return -1;
	}
	return 0;
}

int Request::convert_hex_to_dec(const std::string &hex)
{
	int result = 0;
	for (size_t i = 0; i < hex.size(); i++)
	{
		char c = hex[i];
		if (c >= '0' && c <= '9')
			result = result * 16 + (c - '0');
		else if (c >= 'a' && c <= 'f')
			result = result * 16 + (c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			result = result * 16 + (c - 'A' + 10);
		else
			return -1;
	}
	return result;
}

int Request::parse_body(const std::string &body, size_t &consumed_bytes)
{
	if (this->getHeaders().find("content-length") != this->getHeaders().end())
	{
		size_t bSize = body.size();
		char *end;
		size_t expected_size = strtoul(this->getHeaders().find("content-length")->second.c_str(), &end, 10);

		if (bSize < expected_size)
		{
			return 1;
		}
		else if (bSize == expected_size)
		{
			this->setBody(body);
			consumed_bytes = expected_size;
			return 0;
		}
		else
		{
			this->setBody(body.substr(0, expected_size));
			consumed_bytes = expected_size;
			return 0;
		}
	}
	else if (this->getHeaders().find("transfer-encoding") != this->getHeaders().end())
	{
		std::string chunked_body;
		size_t pos = 0;
		while (true)
		{
			size_t crlf_pos = body.find("\r\n", pos);
			if (crlf_pos == std::string::npos)
				return 1;
			std::string chunk_size_str = body.substr(pos, crlf_pos - pos);
			int chunk_size = convert_hex_to_dec(chunk_size_str);
			if (chunk_size < 0)
				return -1; // Invalid chunk size
			pos = crlf_pos + 2;
			if (body.size() < pos + chunk_size + 2)
				return 1; // Incomplete chunk data
			chunked_body += body.substr(pos, chunk_size);
			pos += chunk_size + 2; // Skip chunk data and trailing CRLF
			if (chunk_size == 0)
				break; // Last chunk
		}
		this->setBody(chunked_body);
		consumed_bytes = pos;
		return 0;
	}
	else
	{
		this->setBody("");
		consumed_bytes = 0;
		return 0;
	}
}

int Request::validateRequest(void)
{
	if (this->getVersion() != "HTTP/1.1")
		return 505;
	if (this->getMethod() == UNKNOWN)
		return 501;
	if (this->getUri().size() > MAX_URI_LENGTH)
		return 414;
	if (Client_max_body_size < this->getBody().size())
		return 413;
	if (this->getHeaders().find("host")->second.empty())
		return 400;
	return 200;
}

int Request::parse_request(std::string &raw)
{
	// NO MORE WHILE LOOP. We only parse ONE request per call.
	if (raw.empty())
		return 1; // Waiting for data

	size_t pos = raw.find("\r\n\r\n");
	size_t pos_req_line = raw.find("\r\n");

	if (pos_req_line == std::string::npos || pos == std::string::npos)
		return 1; // 1 means "Waiting for the rest of the headers..."

	std::string request_line = raw.substr(0, pos_req_line);
	std::string header = raw.substr(pos_req_line + 2, pos - (pos_req_line + 2));

	if (header.size() > MAX_HEADER_SIZE)
		return -1; // 431 Error

	int typeOfError = this->parse_request_line(request_line);
	int typeOfError2 = this->parse_request_headers(header);

	if (typeOfError < 0 || typeOfError2 < 0)
		return -1; // 400 Bad Request

	size_t consumed_body_bytes = 0;
	std::string body = raw.substr(pos + 4);
	int bodyParseResult = this->parse_body(body, consumed_body_bytes);

	if (bodyParseResult < 0)
		return -1; // 400 Bad Request
	if (bodyParseResult == 1)
		return 1; // Waiting for more body data
	// --- SUCCESS! WE HAVE PARSED EXACTLY ONE REQUEST ---
	this->display();
	// 1. Validate it
	// int validation_status = this->validateRequest();
	// (You can store this status inside the req object for the Response to use later)

	// 2. Erase ONLY this one request from the raw string.
	// If there are 2 more requests behind it, they stay in 'raw' for next time!
	size_t total_parsed_bytes = (pos + 4) + consumed_body_bytes;
	raw.erase(0, total_parsed_bytes);
	return 0; // Tell the server: "I have 1 request ready to execute!"
}

// int parse_request(std::string &raw, Request &req)
// {
// 	while (!raw.empty())
// 	{
// 		size_t pos = raw.find("\r\n\r\n");
// 		size_t pos_req_line = raw.find("\r\n");
// 		if (pos_req_line == std::string::npos || pos == std::string::npos)
// 		{
// 			std::cout << "Waiting for the rest of the headers..." << std::endl;
// 			return 1;
// 		}
// 		std::string request_line = raw.substr(0, pos_req_line);
// 		std::string header = raw.substr(pos_req_line + 2, pos - (pos_req_line + 2));
// 		if (header.size() > MAX_HEADER_SIZE)
// 		{
// 			std::cout << "431 Request Header Fields Too Large" << std::endl;
// 			return -1;
// 		}
// 		int typeOfError = parse_request_line(request_line, req);
// 		int typeOfError2 = parse_request_headers(header, req);
// 		if (typeOfError < 0 || typeOfError2 < 0)
// 		{
// 			std::cout << "400 Bad Request: Malformed HTTP" << std::endl;
// 			return -1;
// 		}
// 		size_t consumed_body_bytes = 0;
// 		std::string body = raw.substr(pos + 4);
// 		int bodyParseResult = parse_body(body, req, consumed_body_bytes);
// 		if (bodyParseResult < 0)
// 		{
// 			std::cout << "400 Bad Request: Malformed HTTP" << std::endl;
// 			return -1;
// 		}
// 		if (bodyParseResult == 1)
// 		{
// 			std::cout << "Waiting for more data to complete the body..." << std::endl;
// 			return 1;
// 		}
// 		req.display();
// 		int validation_status = validateRequest(req);
// 		std::cout << "Validation result: " << validation_status << std::endl;
// 		if (validation_status == 200 && req.getMethod() == GET)
// 		{
// 			std::string path = req.getPath();
// 			if (path.empty())
// 				path = "/";
// 			std::string local_path = build_local_path("www/", path);
// 			int resource_status = check_resource(local_path);
// 			if (resource_status == 200)
// 			{
// 				std::string content = execute_get(local_path);
// 				std::cout << content << std::endl;
// 			}
// 			else if (resource_status == 300)
// 			{
// 				if (path[path.size() - 1] != '/')
// 					std::cout << "301 Moved Permanently" << std::endl;
// 				else
// 				{
// 					// autoindexing logic not working yet :(
// 					std::string index_path = build_local_path(local_path, "index.html");
// 					if (check_resource(index_path) == 200)
// 						std::cout << execute_get(index_path) << std::endl;
// 					else
// 						std::cout << "403 Forbidden" << std::endl;
// 				}
// 			}
// 			else
// 				std::cout << "404 Not Found" << std::endl;
// 		}
// 		size_t total_parsed_bytes = (pos + 4) + consumed_body_bytes;
// 		raw.erase(0, total_parsed_bytes);
// 		req = Request();
// 	}
// 	return 0;
// }

int main()
{
	Request req;
	req.parse_request(raw);
	Response res;
	res.handleRequest(req);

	// 	if ( result == -1)
	// 		return -1;
	// 	else if (result == 1)
	// 	{
	// 	std::cout << "Waiting for more data to complete the request..." << std::endl;
	// }

	// if (req.getMethod() == GET)
	// {
	// 	res.get()
	// }

	// std::cout << "Validation result: " << validateRequest(req) << std::endl;
	//	req.display();
}
