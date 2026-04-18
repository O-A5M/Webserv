#include <string>
#include <map>

class	Responce {
private:
	int									status_code;
	std::string							body;
	std::map<std::string, std::string>	headers;

public:
	Responce(void);
	~Responce(void);
};
