#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Request.hpp"
#include "serverConfig.hpp"
#include <string>
#include <map>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <ctime>

struct RouteContext
{
	int status;
	std::string filesystem_path;
	const LocationConfig *matched_location;
	const ServerConfig *matched_server;
	bool is_file;
	bool is_cgi;
	bool is_autoindex;
	bool is_redirect;
	std::vector<std::string> allow_methods;
	std::string reason;

	// C++98 Constructor to set safe defaults
	RouteContext() : status(200), matched_location(NULL), matched_server(NULL),
									 is_file(false), is_cgi(false), is_autoindex(false),
									 is_redirect(false) {}
};

class Response
{
private:
	int status_code;
	std::string reason_phrase;
	std::map<std::string, std::string> headers;
	std::string body;
	std::string raw_response;
	std::string build_local_path(const std::string &root, const std::string &req_path);
	int check_resource(const std::string &local_path);
	void handleGet(const Request &req, const RouteContext mog);
	void handlePost(const Request &req);
	void handleDelete(const Request &req);
	void generateErrorResponse(int code);

public:
	void setStatusCode(int code);
	void setReasonPhrase(const std::string &phrase);
	void setHeaders(const std::map<std::string, std::string> &hdrs);
	void setHeader(const std::string &key, const std::string &value);
	void setBody(const std::string &content);
	void setRawResponse(const std::string &response);

	int getStatusCode() const;
	const std::string &getReasonPhrase() const;
	const std::map<std::string, std::string> &getHeaders() const;
	const std::string &getBody() const;
	const std::string &getRawResponse() const;
	std::string get_mime_type(const std::string &path);
	void handleRequest(const Request &req, const RouteContext mog);
	std::string current_http_date();
	void buildRawResponse();
};


#endif
