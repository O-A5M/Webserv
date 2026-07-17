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
#include "RouteResult.hpp"

#define MAX_URI_LENGTH 8192
#define Client_max_body_size 1000000
#define MAX_HEADER_SIZE 8192
#define RAM_LIMIT 1048576

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

class Request
{

private:
	e_Methodes Method;
	ParseStatus parse_status;
	// ParseResult parse_result;
	std::string uri;
	std::string version;
	std::string query_string;
	std::string path;
	std::string boundary;
	std::map<std::string, std::string> headers;
	std::map<std::string, std::string> cookies; // ADD about (Cookie) parsing
	std::string body;
	// bool complete;
	std::string body_file_path;
	std::string generate_unique_filename();
	size_t body_bytes_processed;

public:
	std::string trim_cookie_part(const std::string &value) const; // ADD about (Cookie) parsing
	void parse_cookies(void);									  // ADD about (Cookie) parsing
	int parse_request_line(const std::string &req_line);
	int parse_request_headers_helper(const std::string &header, size_t startIndex);
	void skip_whitespace(const std::string &header, size_t &i);
	int parse_request_headers(const std::string &header);
	int convert_hex_to_dec(const std::string &hex);
	int parse_body(const std::string &body, size_t &consumed_bytes);

	RouteResult route_result;
	void setMethod(e_Methodes method);
	void setBoundary(const std::string &boundary);
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
	std::string getHeader(const std::string &key) const;
	const std::string &getBoundary(void) const;
	const std::string &getVersion(void) const;
	const std::string &getQuery(void) const;
	const std::string &getPath(void) const;
	const std::string &getBody(void) const;
	const std::map<std::string, std::string> &getHeaders(void) const;
	const std::map<std::string, std::string> &getCookies(void) const; // ADD about (Cookie) parsing
	void removeHeader(const std::string &key);
	void display(void) const;
	bool is_traversal_attempt(const std::string &path);
	ParseStatus validateRequest();
	const std::string &getBodyFilePath() const { return body_file_path; }
};

#endif
