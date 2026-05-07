#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "../../inc/include.hpp"

typedef enum e_Methodes {
	M_GET,
	M_POST,
	M_DELETE
} e_Methodes;

class	Request {

private:
	e_Methodes							Methodes;
	std::string							uri;
	std::string							version;
	std::string							query_string;
	std::string							content_type;
	int			 						content_size;
	std::string							path;
	std::map<std::string, std::string>	headers;
	std::string							body;

public:
	Request(void);
	~Request(void);
	e_Methodes	&getMethodes(void);
	std::string	&getUri(void);
	std::string	&getVersion(void);
	std::string	&getQuery(void);
	std::string	&getType(void);
	int			getSize(void);
	std::string	&getPath(void);
	std::string	&getBody(void);
	std::map
	<std::string,
	std::string>	&getHeaders(void);
};
#endif