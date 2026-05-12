#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <cctype>
#include <cstdlib>


#define MAX_URI_LENGTH 8192
#define Client_max_body_size 1000000
#define MAX_HEADER_SIZE 8192

enum e_Methodes {
    UNKNOWN,
    GET,
    POST,
    DELETE
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

class	Request {

private:
	e_Methodes							Method;
	std::string							uri;
	std::string							version;
	std::string							query_string;
	std::string							path;
	std::map<std::string, std::string>	headers;
	std::string							body;

public:
	void setMethod(e_Methodes method);
	void setUri(const std::string &uri);
	void setVersion(const std::string &version);
	void setQuery(const std::string &query);
	void setPath(const std::string &path);\
	void setBody(const std::string &body);

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
	void removeHeader(const std::string& key);
	void display(void) const;
};
