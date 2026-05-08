#include "Request.hpp"

std::string raw = 
    "POST /api/save HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: 13\r\n"
    "\r\n"
    "name=walid&id=1";

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
	std::cout << "method: " << req.getMethod() << std::endl;
	std::cout << "uri: " << req.getUri() << std::endl;
	std::cout << "version: " << req.getVersion() << std::endl;
	std::cout << "query: " << req.getQuery() << std::endl;
	std::cout << "path: " << req.getPath() << std::endl;
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
	else if (colonFlag)
	{
		return -1;
	}
	return 0;
}

int parse_body(const std::string &body, Request &req)
{
	
	return 0;
}



void parse_request(const std::string &raw, Request &req)
{
	size_t pos = raw.find("\r\n\r\n");
	size_t pos_req_line = raw.find("\r\n");
	if(pos_req_line == std::string::npos || pos == std::string::npos) {
        std::cout << "400 Bad Request: Malformed HTTP" << std::endl;
        return;
    }
	std::string request_line = raw.substr(0 , pos_req_line);
	std::string header = raw.substr(pos_req_line + 2, pos - (pos_req_line + 2));
	std::string body = raw.substr(pos + 4);
	std::cout << "request line : " << request_line << std::endl;
	int typeOfError = parse_request_line(request_line , req);
	std::cout << "return value: " << typeOfError << std::endl;
	std::cout << "------------------------ " << std::endl;
	int typeOfError2 = parse_request_headers(header , req);
	//std::cout << "header: " << header << std::endl;
	if (typeOfError2 != -1)
	std::cout << "headers: " << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = req.getHeaders().begin(); it != req.getHeaders().end(); ++it)
	{
		std::cout << it->first << ":" << it->second << std::endl;
	}
	std::cout << "return value: " << typeOfError2 << std::endl;
	std::cout << "------------------------ " << std::endl;
}
int main()
{
	Request req;
	parse_request(raw , req);
}
