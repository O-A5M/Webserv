#include "Responce.hpp"
#include <fstream>
#include <sstream>

std::string build_local_path(const std::string &root, const std::string &req_path)
{
	std::string final_path = root;
	if (!final_path.empty() && final_path[final_path.size() - 1] == '/')
		final_path.erase(final_path.size() - 1);

	if (!req_path.empty() && req_path[0] != '/')
		final_path += "/";

	final_path += req_path;
	return final_path;
}

int check_resource(const std::string &local_path)
{
	struct stat file_info;
	if (stat(local_path.c_str(), &file_info) != 0)
		return 404;
	if (S_ISDIR(file_info.st_mode))
		return 300;
	return 200;
}

std::string execute_get(const std::string &real_file_path)
{
	std::ifstream file(real_file_path.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open())
		return "";
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}
