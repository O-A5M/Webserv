#include "../../inc/Request.hpp"
#include "../../inc/Response.hpp"

// // ADD about (Cookie) parsing
std::string Request::trim_cookie_part(const std::string &value) const
{
	size_t start = 0;
	size_t end = value.size();

	while (start < end && (value[start] == ' ' || value[start] == '\t'))
		start++;
	while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t'))
		end--;
	return value.substr(start, end - start);
}

// ADD about (Cookie) parsing
void Request::parse_cookies(void)
{
	this->cookies.clear();

	std::map<std::string, std::string>::const_iterator it = this->headers.find("cookie");
	if (it == this->headers.end())
		return;

	const std::string &cookie_header = it->second;
	size_t start = 0;
	while (start <= cookie_header.size())
	{
		size_t end = cookie_header.find(';', start);
		std::string cookie_pair;

		if (end == std::string::npos)
			cookie_pair = cookie_header.substr(start);
		else
			cookie_pair = cookie_header.substr(start, end - start);

		size_t equal_pos = cookie_pair.find('=');
		if (equal_pos != std::string::npos)
		{
			std::string name = trim_cookie_part(cookie_pair.substr(0, equal_pos));
			std::string value = trim_cookie_part(cookie_pair.substr(equal_pos + 1));
			if (!name.empty())
				this->cookies[name] = value;
		}

		if (end == std::string::npos)
			break;
		start = end + 1;
	}
}

bool Request::is_traversal_attempt(const std::string &path)
{
	if (path.find("/../") != std::string::npos)
		return true;
	if (path.find("../") == 0)
		return true;
	if (path.length() >= 3 && path.substr(path.length() - 3) == "/..")
		return true;
	if (path == "..")
		return true;
	return false;
}

std::string decodeURI(const std::string& encoded) {
    std::string decoded;
    decoded.reserve(encoded.length()); 

    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            
            std::string hexStr = encoded.substr(i + 1, 2);
            
            
            char decodedChar = static_cast<char>(std::strtol(hexStr.c_str(), NULL, 16));
            
            decoded += decodedChar;
            i += 2; 
        } 
        else if (encoded[i] == '+') {
            
            decoded += ' ';
        } 
        else {
            
            decoded += encoded[i];
        }
    }
    return decoded;
}
int Request::parse_request_line(const std::string &req_line)
{
	std::string method_str;
	std::string uri;
	std::string version;
	int space_count = 0;
	if (req_line.empty() || req_line[0] == ' ')
		return -1;
	for (size_t i = 0; i < req_line.size(); i++)
	{
		if (req_line[i] == '\r')
			continue;
		if (req_line[i] == ' ')
		{
			if (i > 0 && req_line[i - 1] != ' ')
			{
				space_count++;
			}
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
	if (method_str.empty() || uri.empty() || version.empty())
		return -2;
	if (method_str == "GET")
		this->setMethod(GET);
	else if (method_str == "POST")
		this->setMethod(POST);
	else if (method_str == "DELETE")
		this->setMethod(DELETE);
	else
		this->setMethod(UNKNOWN);
	if (!uri.empty())
	{
	std::string SafeUri = decodeURI(uri);
	this->setUri(SafeUri);
	size_t pos = SafeUri.find("?");
	if (pos != std::string::npos)
		this->setQuery(SafeUri.substr(pos + 1, SafeUri.size() - pos - 1));
	this->setPath(SafeUri.substr(0, pos));
	}
	this->setVersion(version);
	// std::cout << "Parsed Request Line: Method=" << method_str << ", URI=" << uri << ", Version=" << version << std::endl;
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
	std::string contentType = getHeader("content-type");
	if (!contentType.empty())
	{
	if (contentType.find("multipart/form-data") != std::string::npos)
	{
		size_t boundaryPos = contentType.find("boundary=");
		if (boundaryPos != std::string::npos)
		{
			std::string boundary = "--" + contentType.substr(boundaryPos + 9);
			setBoundary(boundary);
		}
		else
		{
			std::cout << "Boundary not found in Content-Type header" << std::endl;
			return -1;
		}
	}
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
	this->parse_cookies();
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


std::string Request::generate_unique_filename()
{
	static unsigned long counter = 0;
	std::stringstream ss;

	ss << "/tmp/body_"
		 << std::time(NULL) << "_"
		 << ++counter << "_"
		 << std::rand() << ".bin";

	return ss.str();
}

int Request::parse_body(const std::string &body, size_t &consumed_bytes)
{
	if (this->getHeaders().find("content-length") != this->getHeaders().end())
	{
		if (this->body_file_path.empty())
			this->body_file_path = generate_unique_filename();

		std::ofstream body_file(this->body_file_path.c_str(), std::ios::binary | std::ios::app);
		if (!body_file.is_open())
			return -1;

		size_t bSize = body.size();
		char *end;
		size_t expected_size = strtoul(this->getHeaders().find("content-length")->second.c_str(), &end, 10);
		if (this->body_bytes_processed > bSize)
		{
			body_file.close();
			return -1;
		}

		size_t new_bytes = bSize - this->body_bytes_processed;

		if (new_bytes > 0)
		{
			size_t bytes_to_write = new_bytes;
			if (this->body_bytes_processed + new_bytes > expected_size)
				bytes_to_write = expected_size - this->body_bytes_processed;

			body_file.write(body.data() + this->body_bytes_processed, bytes_to_write);

			this->body_bytes_processed += bytes_to_write;
		}

		if (this->body_bytes_processed < expected_size)
		{
			body_file.close();
			return 1; 
		}

		body_file.close();
		consumed_bytes = expected_size;
		return 0; 
	}

	else if (this->getHeaders().find("transfer-encoding") != this->getHeaders().end())
	{
		if (this->body_file_path.empty())
			this->body_file_path = generate_unique_filename();

		std::ofstream body_file(this->body_file_path.c_str(), std::ios::binary | std::ios::app);
		if (!body_file.is_open())
			return -1;

		size_t pos = this->body_bytes_processed;

		while (true)
		{
			size_t crlf_pos = body.find("\r\n", pos);
			if (crlf_pos == std::string::npos)
			{
				body_file.close();
				return 1; 
			}

			std::string chunk_size_str = body.substr(pos, crlf_pos - pos);
			int chunk_size = convert_hex_to_dec(chunk_size_str);

			if (chunk_size < 0)
			{
				body_file.close();
				return -1; 
			}

			size_t data_start = crlf_pos + 2;

			if (body.size() < data_start + chunk_size + 2)
			{
				body_file.close();
				return 1; 
			}

			if (chunk_size > 0)
			{
				body_file.write(body.data() + data_start, chunk_size);
			}

			pos = data_start + chunk_size + 2;

			this->body_bytes_processed = pos;

			if (chunk_size == 0)
				break; 
		}

		body_file.close();
		consumed_bytes = pos;
		return 0;
	}
	else
	{
		consumed_bytes = 0;
		return 0;
	}
}
ParseStatus Request::validateRequestLine()
{
	std::cout << "Validating Request Line: Method=" << this->getMethod() << ", URI=" << this->getUri() << ", Version=" << this->getVersion() << std::endl;
	if (this->getMethod() == UNKNOWN)
	{
		return METHOD_NOT_ALLOWED;
	}
	if (this->getUri().size() > MAX_URI_LENGTH)
		return URI_TOO_LONG;
	if (this->is_traversal_attempt(this->getUri()))
		return FORBIDDEN;
	if (this->getVersion() != "HTTP/1.1")
		return VERSION_NOT_SUPPORTED;
	return OK;
}
ParseStatus Request::validateHeaders()
{
	
	std::map<std::string, std::string>::const_iterator it = this->getHeaders().find("host");
	if (it == this->getHeaders().end() || it->second.empty())
		return BAD_REQUEST;
	return OK;
}