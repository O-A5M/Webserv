#include "../../inc/Response.hpp"

std::string Response::build_local_path(const std::string &root, const std::string &req_path)
{
	std::string final_path = root;
	if (!final_path.empty() && final_path[final_path.size() - 1] == '/')
		final_path.erase(final_path.size() - 1);

	if (!req_path.empty() && req_path[0] != '/')
		final_path += "/";

	final_path += req_path;
	return final_path;
}

int Response::check_resource(const std::string &local_path)
{
	std::cout << "Resource status for " << local_path << std::endl;
	struct stat file_info;
	if (stat(local_path.c_str(), &file_info) != 0)
		return 404;
	if (S_ISDIR(file_info.st_mode))
		return 300;
	if (access(local_path.c_str(), R_OK) != 0)
	{
		std::cout << "Permission denied for: " << local_path << std::endl;
		return 403; // Forbidden
	}
	return 200;
}

bool is_traversal_attempt(const std::string &path)
{
	if (path.find("/../") != std::string::npos)
		return true;
	if (path.find("../") == 0)
		return true;
	if (path.length() >= 3 && path.substr(path.length() - 3) == "/..")
		return true;
	if (path == "..")
		return true;
	return false;
}

std::string Response::get_mime_type(const std::string &path)
{
	size_t dot_pos = path.find_last_of('.');
	if (dot_pos == std::string::npos)
		return "application/octet-stream";
	std::string ext = path.substr(dot_pos);
	if (ext == ".html" || ext == ".htm")
		return "text/html";
	if (ext == ".css")
		return "text/css";
	if (ext == ".js")
		return "application/javascript";
	if (ext == ".jpg" || ext == ".jpeg")
		return "image/jpeg";
	if (ext == ".png")
		return "image/png";
	if (ext == ".gif")
		return "image/gif";
	if (ext == ".txt")
		return "text/plain";
	return "application/octet-stream";
}

void Response::buildRawResponse()
{
	std::stringstream response_stream;
	response_stream << "HTTP/1.1 " << this->status_code << " " << this->reason_phrase << "\r\n";
	std::map<std::string, std::string>::const_iterator it;
	for (it = this->headers.begin(); it != this->headers.end(); ++it)
		response_stream << it->first << ": " << it->second << "\r\n";
	response_stream << "\r\n";
	response_stream << this->body;
	this->setRawResponse(response_stream.str());
}

std::string Response::current_http_date()
{
	char buffer[100];
	time_t now = std::time(NULL);
	struct tm *tm_info = gmtime(&now);
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
	return std::string(buffer);
}
void Response::handleGet(const Request &req, const RouteContext mog)
{
	std::cout << mog.status <<  std::endl;
	std::cout << mog.filesystem_path <<  std::endl;
	(void)req;

	// std::string path = req.getPath();
	// if (path  == "/")
		// path = "/" + server_conf.index[0];
	// if (is_traversal_attempt(path))
	// {
		// generateErrorResponse(403);
		std::cout << " hack attack " << std::endl;
		return;
	// }
			std::string local_path = build_local_path(server_conf.root, path);
			int resource_status = check_resource(local_path);
			std::cout << "Resource status for " << local_path << ": " << resource_status << std::endl;
			if (resource_status == 200)
			{
				std::ifstream file(local_path.c_str(), std::ios::in | std::ios::binary);
				if (!file.is_open())
				{
					// 	generateErrorResponse(404);
					// 	return;
					std::cout << "Failed to open file: " << local_path << std::endl;
					return;
				}
				std::stringstream buffer;
				buffer << file.rdbuf();
				this->setStatusCode(200);
				this->setReasonPhrase("OK");
				this->setBody(buffer.str());
				this->setHeader("Content-Type", get_mime_type(local_path));
				std::stringstream buuferLenght;
				buuferLenght << this->getBody().size();
				this->setHeader("Content-Length", buuferLenght.str());
				this->setHeader("Date", this->current_http_date());
				this->buildRawResponse();
				std::cout << this->getRawResponse() << std::endl;
			}
			else if (resource_status == 300)
			{
					if (path[path.size() - 1] != '/')
					{
						this->setStatusCode(301);
						this->setReasonPhrase("Moved Permanently");
						this->setHeader("Location", path + "/");
						this->buildRawResponse();
						std::cout << this->getRawResponse() << std::endl;
					}
				}
}
void Response::handleRequest(const Request &req, const RouteContext mog)
{
	if (req.getMethod() == GET)
	{
		handleGet(req, mog);
	}
	// else if (req.getMethod() == POST)
	// 	handlePost(req);

	// else if (req.getMethod() == DELETE)
	// 	handleDelete(req);

	// else
	// 	generateErrorResponse(405);
}
