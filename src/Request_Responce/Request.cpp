#include "Request.hpp"

std::string raw =
		"POST /api/life?walid=walid&id=1 HTTP/1.1\r\n"
		"Host: google.com\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: 16\r\n"
		"Content-Length: 16\r\n"
		"\r\n"
		"walid=walid&id=1"
		"GET /api/save?walid=walid&id=3 HTTP/1.1\r\n"
		"Host: facebook.com\r\n"
		"Content-Type: application/html\r\n"
		"\r\n"
		"GET /api/knight?walid=walid&id=2 HTTP/1.1\r\n"
		"Host: youtube.com\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"\r\n"
		"POST /api/upload HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"Content-Type: text/plain\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"1A\r\n"
		"This is a chunked request."
		"\r\n"
		"11\r\n"
		" It is very cool."
		"\r\n"
		"0\r\n"
		"\r\n";

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
		if (req_line[i] == '\r')
			continue;
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
	size_t pos = uri.find("?");
	if (pos != std::string::npos)
		req.setQuery(uri.substr(pos + 1 , uri.size() - pos - 1));
	req.setPath(uri.substr(0, pos));
	req.setVersion(version);
	return 0;
}

int parse_request_headers_helper(const std::string &header , int startIndex)
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

void skip_whitespace(const std::string &header, size_t &i)
{
	while (i < header.size() && (header[i] == ' ' || header[i] == '\t'))
		i++;
}
int parse_request_headers(const std::string &header, Request &req)
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
			while (i < header.size() && header[i] != ':' && header[i] != '\n' )
			{
				if (header[i] == ' ' || header[i] == '\t')
					return -2;
				key += std::tolower(header[i++]);
			}
			if (header[i] != ':') return -1;
			i++;
			std::string value;

			skip_whitespace(header, i);
			while (i < header.size() && header[i] != '\n')
			{
				if (header[i] == ' ' || header[i] == '\t' )
				{
					if (parse_request_headers_helper(header , i) == 0)
					{
						i++;
						continue;
					}
				}
				if (header[i] == '\r') { i++; continue; }
				value += header[i++];
			}
			errorR = req.setHeader(key, value) ;
			if (errorR < 0)
			{
				if (errorR == -2)
					colonFlag = 1;
				else
					return -1;
			}
		}
	}
	if (req.getHeaders().find("host") == req.getHeaders().end())
	{
    	std::cout << "Host header does not exist" << std::endl;
		return -3;
	}
	if ((req.getHeaders().find("transfer-encoding") != req.getHeaders().end()))
	{
		if (req.getHeaders().find("content-length") != req.getHeaders().end())
		{
			req.removeHeader("content-length");
		}
	}
	else if ((req.getHeaders().find("content-length") != req.getHeaders().end()) && colonFlag == 0)
	{
		char *end;
		long n = std::strtol(req.getHeaders().find("content-length")->second.c_str(), &end, 10);
		if (*end != '\0')
			return -1;
	}
	else if (colonFlag)
	{
		return -1;
	}
	return 0;
}

int convert_hex_to_dec(const std::string &hex)
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

int parse_body(const std::string &body, Request &req, size_t &consumed_bytes)
{
    if (req.getHeaders().find("content-length") != req.getHeaders().end())
    {
        size_t bSize = body.size();
        char *end;
        size_t expected_size = std::strtoul(req.getHeaders().find("content-length")->second.c_str(), &end, 10);

        if (bSize < expected_size)
        {
            return 1; // 1 means "Incomplete, go back to poll/select and wait"
        }
        else if (bSize == expected_size)
        {
            req.setBody(body);
            consumed_bytes = expected_size;
            return 0; // 0 means "Perfect, request is ready!"
        }
        else
        {
            req.setBody(body.substr(0, expected_size));
            consumed_bytes = expected_size;
            return 0;
        }
    }
		else if (req.getHeaders().find("transfer-encoding") != req.getHeaders().end())
		{
			std::string chunked_body;
			size_t pos = 0;
			while (true)
			{
				size_t crlf_pos = body.find("\r\n", pos);
				if (crlf_pos == std::string::npos)
					return 1; // Incomplete chunk size line
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
			req.setBody(chunked_body);
			consumed_bytes = pos;
			return 0;
		}
		else
		{
				req.setBody("");
				consumed_bytes = 0;
				return 0; // No Content-Length or Transfer-Encoding, treat as complete
		}
		}

int  parse_request(std::string &raw, Request &req)
{
	while (!raw.empty())
    {
		size_t pos = raw.find("\r\n\r\n");
		size_t pos_req_line = raw.find("\r\n");
		if(pos_req_line == std::string::npos || pos == std::string::npos) {
			std::cout << "Waiting for the rest of the headers..." << std::endl;
			return 1;
    	}
		std::string request_line = raw.substr(0 , pos_req_line);
		std::string header = raw.substr(pos_req_line + 2, pos - (pos_req_line + 2));
		int typeOfError = parse_request_line(request_line , req);
		int typeOfError2 = parse_request_headers(header , req);
		if (typeOfError < 0 || typeOfError2 < 0)
		{
			std::cout << "400 Bad Request: Malformed HTTP" << std::endl;
			return -1;
		}
		size_t consumed_body_bytes = 0;
			std::string body = raw.substr(pos + 4);
			int bodyParseResult = parse_body(body, req, consumed_body_bytes);
			if (bodyParseResult < 0)
			{
				std::cout << "400 Bad Request: Malformed HTTP" << std::endl;
				return -1;
			}
			if (bodyParseResult == 1)
			{
				std::cout << "Waiting for more data to complete the body..." << std::endl;
				return 1;
			}
		req.display();
		size_t total_parsed_bytes = (pos + 4) + consumed_body_bytes;
        raw.erase(0, total_parsed_bytes);
		req = Request();
	}
	return 0;
}

int main()
{
	Request req;
	parse_request(raw , req);
	//	req.display();
}
