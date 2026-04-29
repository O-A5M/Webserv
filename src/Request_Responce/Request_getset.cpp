// Implementation of Request member functions
#include "Request.hpp"
#include <string>
#include <map>

void Request::setMethod(e_Methodes method)
{
	this->Method = method;
}

Request::Request(void) : Method(UNKNOWN),
      uri(""),
      version(""),
      query_string(""),
      path(""),
      headers(),
      body("")
{
}
Request::~Request(void)
{
}
void Request::setUri(const std::string &uri)
{
	this->uri = uri;
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

void Request::setHeader(const std::string &key, const std::string &value)
{
	this->headers[key] = value;
}

void Request::setHeaders(const std::map<std::string, std::string> &headers)
{
	this->headers = headers;
}
const e_Methodes &Request::getMethodes(void) const
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
