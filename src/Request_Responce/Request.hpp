#include <string>
#include <map>

#define GET 0
#define POST 1
#define DELETE 2

typedef enum e_Methodes {
	M_GET,
	M_POST,
	M_DELETE
} e_Methodes;

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
