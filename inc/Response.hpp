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
#include <cstdio>
#include <vector>
class Response
{
private:
	int status_code;
	std::string reason_phrase;
	std::map<std::string, std::string> headers;
	std::string body;
	std::string raw_response;

	void handleGet(const Request &req, const RouteResult &context);
	void handlePost(const Request &req , const RouteResult &context);
	void handleDelete(const RouteResult &context , const Request &req);
	// [cookies]
	std::string currentSessionId;

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

	// [cookies]
	void manageGlobalSession(const Request &req);



public:
	void buildRedirectResponse(const RouteResult &context , const Request &req);
	const std::string &getRawResponse() const;
	static Response generateErrorResponse(int code , const RouteResult &context);
	void serveFile(const RouteResult &context , const Request &req);
	void build(const Request &req, const RouteResult &context);

	// about cookies
	void setCookie(const std::string &name, const std::string &value, const std::string &path = "/", bool httpOnly = true);
	std::vector<std::string> setCookieHeaders; // Add this container

	void	buildFromCgi(const std::string &cgiOutput, const RouteResult &context);
};

#endif
