#include <Request.hpp>
#include <string>

Request::Request(void) {
	
}

Request::~Request(void) {
	
}

e_Methodes	&Request::getMethodes(void) {
	return (Methodes);
}

std::string	&Request::getUri(void) {
	return (uri);
}

std::string &Request::getVersion(void) {
	return (version);
}

std::string	&Request::getQuery(void) {
	return (query_string); 
}

std::string &Request::getType(void) {
	return (content_type);
}

int Request::getSize(void) {
	return (content_size);
}

std::string &Request::getPath(void) {
	return (path);
}

std::string	&Request::getBody(void) {
	return (body);
}

std::map<std::string, std::string>	&Request::getHeaders(void) {
	return (headers);
}
