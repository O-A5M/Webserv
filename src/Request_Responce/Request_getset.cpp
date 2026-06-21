// Implementation of Request member functions
#include "../../inc/Request.hpp"

void Request::setMethod(e_Methodes method)
{
	this->Method = method;
}

Request::Request(void) : Method(UNKNOWN),
      parse_status(OK),
      uri(""),
      version(""),
      query_string(""),
      path(""),
      boundary(""),
      headers(),
      body(""),
      body_file_path(""),
      body_bytes_processed(0)
{
}
Request::~Request(void)
{
}
void Request::clear(void)
{
	this->Method = UNKNOWN;
	this->parse_status = OK;
	this->uri.clear();
	this->version.clear();
	this->query_string.clear();
	this->path.clear();
	this->headers.clear();
	this->body.clear();
	this->boundary.clear();
	this->body_file_path.clear();
	this->body_bytes_processed = 0;
	this->route_result = RouteResult();
}
void Request::setUri(const std::string &uri)
{
	this->uri = uri;
}

void Request::setBoundary(const std::string &boundary)
{
	this->boundary = boundary;
}
const std::string &Request::getBoundary(void) const
{
	return this->boundary;
}
void Request::setVersion(const std::string &version)
{
	this->version = version;
}

void Request::setQuery(const std::string &query)
{
	this->query_string = query;
}

void Request::setPath(const std::string &path)
{
	this->path = path;
}

void Request::setBody(const std::string &body)
{
	this->body = body;
}

//void Request::setHeader(const std::string &key, const std::string &value)
//{
//	this->headers[key] = value;
//}

// Returns 0 on success, or -1 (or 400) if the request should be rejected
int Request::setHeader(std::string key,std::string value) {

    // Check if the header already exists in our map
    if (this->headers.find(key) != this->headers.end()) {
        if (key == "host")
            return -1; // Trigger 400 Bad Request
        // EDGE CASE 2: The "Content-Length" Header
        // ---------------------------------------------------------
        if (key == "content-length") {
            // If they are exactly the same, ignore the duplicate.
            if (this->headers[key] == value) {
                return 0;
            }
            return -2; // Trigger 400 Bad Request
        }
        if (key == "content-type") {
            if (this->headers[key] == value) {
                return 0;
            }
            return -1; // Trigger 400 Bad Request
        }
        // ---------------------------------------------------------
        // STANDARD RULE: Combine with a comma
        // ---------------------------------------------------------
        // For Accept, Transfer-Encoding, etc.
        this->headers[key] += ", " + value;

    } else {
        // It's the first time we are seeing this header. Just insert it.
        this->headers[key] = value;
    }

    return 0;
}

std::string Request::getHeader(const std::string &key) const
{
	std::map<std::string, std::string>::const_iterator it = this->headers.find(key);

	if (it != this->headers.end())
	{
		return it->second;
	}

	return "";
}

void Request::setHeaders(const std::map<std::string, std::string> &headers)
{
	this->headers = headers;
}
const e_Methodes &Request::getMethod(void) const
{
	return this->Method;
}

const std::string &Request::getUri(void) const
{
	return this->uri;
}

const std::string &Request::getVersion(void) const
{
	return this->version;
}

const std::string &Request::getQuery(void) const
{
	return this->query_string;
}

const std::string &Request::getPath(void) const
{
	return this->path;
}

const std::string &Request::getBody(void) const
{
	return this->body;
}

const std::map<std::string, std::string> &Request::getHeaders(void) const
{
	return this->headers;
}

void Request::removeHeader(const std::string& key) {
    this->headers.erase(key);
}

// void Request::display(void) const
// {
// 	std::cout << "===== REQUEST =====" << std::endl;
// 	if (this->Method == UNKNOWN)
// 		std::cout << "Method:UNKNOWN" << std::endl;
// 	else if (this->Method == GET)
// 		std::cout << "Method:GET" << std::endl;
// 	else if (this->Method == POST)
// 		std::cout << "Method:POST" << std::endl;
// 	else if (this->Method == DELETE)
// 		std::cout << "Method:DELETE" << std::endl;
// 	std::cout << "URI:" << this->uri << std::endl;
// 	std::cout << "Version:" << this->version << std::endl;
// 	std::cout << "Query String:" << this->query_string << std::endl;
// 	std::cout << "Path:" << this->path << std::endl;

// 	std::cout << "\n--- Headers ---" << std::endl;

// 	std::map<std::string, std::string>::const_iterator it;

// 	for (it = this->headers.begin(); it != this->headers.end(); ++it)
// 	{
// 		std::cout << it->first << ":" << it->second << std::endl;
// 	}

// 	std::cout << "\n--- Body ---" << std::endl;

// 	std::cout << "===================" << std::endl;
// }
#include <fstream> // Make sure this is at the top of your file

void Request::display(void) const
{
	std::cout << "===== REQUEST =====" << std::endl;

	if (this->Method == UNKNOWN)
		std::cout << "Method:UNKNOWN" << std::endl;
	else if (this->Method == GET)
		std::cout << "Method:GET" << std::endl;
	else if (this->Method == POST)
		std::cout << "Method:POST" << std::endl;
	else if (this->Method == DELETE)
		std::cout << "Method:DELETE" << std::endl;

	std::cout << "URI:" << this->uri << std::endl;
	std::cout << "Version:" << this->version << std::endl;
	std::cout << "Query String:" << this->query_string << std::endl;
	std::cout << "Path:" << this->path << std::endl;

	std::cout << "\n--- Headers ---" << std::endl;

	std::map<std::string, std::string>::const_iterator it;
	for (it = this->headers.begin(); it != this->headers.end(); ++it)
	{
		std::cout << it->first << ":" << it->second << std::endl;
	}

	std::cout << "\n--- Body ---" << std::endl;

	// 1. Check if we actually have a body file
	if (this->body_file_path.empty())
	{
		std::cout << "(No body or not yet parsed)" << std::endl;
	}
	else
	{
		// 2. Print where the file is stored
		std::cout << "Location: " << this->body_file_path << std::endl;

		// 3. Open the file to peek at the first 100 bytes
		std::ifstream peek_file(this->body_file_path.c_str(), std::ios::binary);
		if (peek_file.is_open())
		{
			char buffer[100];
			peek_file.read(buffer, sizeof(buffer));
			std::streamsize bytes_read = peek_file.gcount();

			if (bytes_read > 0)
			{
				std::cout << "Preview (first " << bytes_read << " bytes):" << std::endl;
				std::cout << "[";

				// Print characters safely. If it's a binary byte, print a dot '.' instead.
				for (std::streamsize i = 0; i < bytes_read; ++i)
				{
					if (buffer[i] >= 32 && buffer[i] <= 126) // Printable ASCII range
						std::cout << buffer[i];
					else
						std::cout << '.';
				}
				std::cout << "]" << std::endl;

				// If the file is longer than our 100 byte peek, let the user know
				if (!peek_file.eof())
					std::cout << "... (truncated for display)" << std::endl;
			}
			peek_file.close();
		}
		else
		{
			std::cout << "(Could not open file for preview)" << std::endl;
		}
	}

	std::cout << "===================" << std::endl;
}
