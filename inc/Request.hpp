#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <cctype>
#include <cstdlib>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <algorithm>

#define MAX_URI_LENGTH 8192
#define Client_max_body_size 1000000
#define MAX_HEADER_SIZE 8192

enum e_Methodes
{
	UNKNOWN,
	GET,
	POST,
	DELETE
};

enum ParseStatus
{
	OK = 200,
	BAD_REQUEST = 400,
	UNAUTHORIZED = 401,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
	METHOD_NOT_ALLOWED = 405,
	URI_TOO_LONG = 414,
	NOT_IMPLEMENTED = 501,
	PAYLOAD_TOO_LARGE = 413,
	VERSION_NOT_SUPPORTED = 505
};

enum ParseResult
{
	PARSE_WAITING,
	PARSE_SUCCESS,
	PARSE_BAD_REQUEST = 400,		 // 400
	PARSE_HEADER_TOO_LARGE = 431 // 431
};

// typedef  struct	s_Request {
// 	e_Methodes							Methodes;
// 	std::string							uri;
// 	std::string							version;
// 	std::string							content_type;
// 	std::string 						content_size;
// 	std::string							path;
// 	std::map<std::string, std::string>	headers;
// 	std::string							body;
// } s_Request;

class Request
{

private:
	e_Methodes Method;
	ParseStatus parse_status;
	ParseResult parse_result;
	std::string uri;
	std::string version;
	std::string query_string;
	std::string path;
	std::map<std::string, std::string> headers;
	std::string body;
	// bool complete;

	int parse_request_line(const std::string &req_line);
	int parse_request_headers_helper(const std::string &header, size_t startIndex);
	void skip_whitespace(const std::string &header, size_t &i);
	int parse_request_headers(const std::string &header);
	int convert_hex_to_dec(const std::string &hex);
	int parse_body(const std::string &body, size_t &consumed_bytes);

public:
	void setMethod(e_Methodes method);
	void setUri(const std::string &uri);
	void setVersion(const std::string &version);
	void setQuery(const std::string &query);
	void setPath(const std::string &path);
	void setBody(const std::string &body);
	void clear(void);

	int setHeader(std::string key, std::string value);
	void setHeaders(const std::map<std::string, std::string> &headers);
	Request(void);
	~Request(void);
	const e_Methodes &getMethod(void) const;
	const std::string &getUri(void) const;
	const std::string &getVersion(void) const;
	const std::string &getQuery(void) const;
	const std::string &getPath(void) const;
	const std::string &getBody(void) const;
	const std::map<std::string, std::string> &getHeaders(void) const;
	void removeHeader(const std::string &key);
	void display(void) const;
	ParseStatus validateRequest(void);
	int parse_request(std::string &raw);
};

#endif
