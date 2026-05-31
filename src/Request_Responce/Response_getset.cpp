#include "Response.hpp"

void Response::setStatusCode(int code)
{
	status_code = code;
}

void Response::setReasonPhrase(const std::string &phrase)
{
	reason_phrase = phrase;
}

void Response::setHeaders(const std::map<std::string, std::string> &hdrs)
{
	headers = hdrs;
}

void Response::setHeader(const std::string &key, const std::string &value)
{
	headers[key] = value;
}

void Response::setBody(const std::string &content)
{
	body = content;
}

void Response::setRawResponse(const std::string &response)
{
	raw_response = response;
}

int Response::getStatusCode() const
{
	return status_code;
}

const std::string &Response::getReasonPhrase() const
{
	return reason_phrase;
}

const std::map<std::string, std::string> &Response::getHeaders() const
{
	return headers;
}

const std::string &Response::getBody() const
{
	return body;
}

const std::string &Response::getRawResponse() const
{
	return raw_response;
}
