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
#include <cerrno>
#include "RouteResult.hpp"
#include <dirent.h>
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
	void handleGet(const Request &req, const RouteResult &context);
	void handlePost(const Request &req);
	void handleDelete(const Request &req);

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
	std::string get_mime_type(const std::string &path);
	std::string current_http_date();
	void buildRawResponse();
	void dispatchMethod(const Request &req, const RouteResult &context);
	static std::string buildErrorPage(int code, const std::string &reason);
	std::string buildAutoIndex(const std::string &physicalPath, const std::string &requestURI);

public:
	void buildRedirectResponse(const RouteResult &context);
	const std::string &getRawResponse() const;
	static Response generateErrorResponse(int code);
	void serveFile(const RouteResult &context);
	void build(const Request &req, const RouteResult &context);
};

#endif
