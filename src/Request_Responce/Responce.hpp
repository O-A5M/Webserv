#include <string>
#include <map>
#include <sys/stat.h>

// class	Responce {
// private:
// 	int									status_code;
// 	std::string							body;
// 	std::map<std::string, std::string>	headers;

// public:
// 	Responce(void);
// 	~Responce(void);
// };

std::string build_local_path(const std::string &root, const std::string &req_path);
int check_resource(const std::string &local_path);
std::string execute_get(const std::string &real_file_path);
