#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Request.hpp"
#include <string>
#include <map>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

class Response
{
	private:
		int status_code;
		std::string reason_phrase;
		std::map<std::string, std::string> headers;
		std::string body;
		std::string raw_response;
		void  handleGet(const Request &req);
		void  handlePost(const Request &req);
		void  handleDelete(const Request &req);
		void  generateErrorResponse(int code);

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
		void handleRequest(const Request &req);
};

std::string build_local_path(const std::string &root, const std::string &req_path);
int check_resource(const std::string &local_path);
std::string execute_get(const std::string &real_file_path);

#endif
