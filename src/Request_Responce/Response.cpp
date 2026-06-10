#include "../../inc/Response.hpp"

// std::string Response::build_local_path(const std::string &root, const std::string &req_path)
// {
// 	std::string final_path = root;
// 	if (!final_path.empty() && final_path[final_path.size() - 1] == '/')
// 		final_path.erase(final_path.size() - 1);

// 	if (!req_path.empty() && req_path[0] != '/')
// 		final_path += "/";

// 	final_path += req_path;
// 	return final_path;
// }

// int Response::check_resource(const std::string &local_path)
// {
// 	std::cout << "Resource status for " << local_path << std::endl;
// 	struct stat file_info;
// 	if (stat(local_path.c_str(), &file_info) != 0)
// 		return 404;
// 	if (S_ISDIR(file_info.st_mode))
// 		return 300;
// 	if (access(local_path.c_str(), R_OK) != 0)
// 	{
// 		std::cout << "Permission denied for: " << local_path << std::endl;
// 		return 403; // Forbidden
// 	}
// 	return 200;
// }

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

std::string Response::buildErrorPage(int code, const std::string &reason)
{
	std::ostringstream oss;
	oss << code;

	std::string body =
			"<!DOCTYPE html>"
			"<html>"
			"<head>"
			"<meta charset=\"UTF-8\">"
			"<title>" +
			oss.str() + " " + reason + "</title>"
																 "<style>"
																 "body {"
																 "margin: 0;"
																 "font-family: Arial, sans-serif;"
																 "background: #f5f5f5;"
																 "color: #333;"
																 "display: flex;"
																 "justify-content: center;"
																 "align-items: center;"
																 "height: 100vh;"
																 "}"
																 ".box {"
																 "text-align: center;"
																 "padding: 40px;"
																 "background: white;"
																 "border-radius: 10px;"
																 "box-shadow: 0 10px 30px rgba(0,0,0,0.1);"
																 "max-width: 500px;"
																 "}"
																 "h1 {"
																 "font-size: 48px;"
																 "margin: 0;"
																 "color: #e74c3c;"
																 "}"
																 "p {"
																 "font-size: 18px;"
																 "margin-top: 10px;"
																 "}"
																 ".code {"
																 "font-size: 14px;"
																 "color: #888;"
																 "margin-top: 20px;"
																 "}"
																 "</style>"
																 "</head>"
																 "<body>"
																 "<div class=\"box\">"
																 "<h1>" +
			oss.str() + "</h1>"
									"<p>" +
			reason + "</p>"
							 "<div class=\"code\">Webserv Server</div>"
							 "</div>"
							 "</body>"
							 "</html>";

	return body;
}

Response Response::generateErrorResponse(int code)
{
	Response res;
	std::string reason;

	switch (code)
	{
	case 400:
		reason = "Bad Request";
		break;
	case 403:
		reason = "Forbidden";
		break;
	case 404:
		reason = "Not Found";
		break;
	case 405:
		reason = "Method Not Allowed";
		break;
	case 413:
		reason = "Payload Too Large";
		break;
	case 414:
		reason = "URI Too Long";
		break;
	case 431:
		reason = "Request Header Fields Too Large";
		break;
	case 500:
		reason = "Internal Server Error";
		break;
	case 501:
		reason = "Not Implemented";
		break;
	case 505:
		reason = "HTTP Version Not Supported";
		break;
	default:
		reason = "Unknown Error";
		break;
	}

	std::ostringstream oss;
	oss << code;
	std::string code_str = oss.str();

	std::string body = buildErrorPage(code, reason);

	res.setStatusCode(code);
	res.setReasonPhrase(reason);

	res.setHeader("Content-Type", "text/html");

	std::ostringstream len;
	len << body.size();
	res.setHeader("Content-Length", len.str());

	res.setHeader("Connection", "close");

	res.setBody(body);

	res.buildRawResponse();

	return res;
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
void Response::handleGet(const Request &req, const RouteResult &mog)
{

	(void)req;
	if (std::find(mog.allow_methods.begin(), mog.allow_methods.end(), "GET") == mog.allow_methods.end())
	{
		*this = generateErrorResponse(405);
		return;
	}
	// std::cout << this->getRawResponse() << std::endl;
	// std::string path = req.getPath();
	// if (path  == "/")
		// path = "/" + server_conf.index[0];
	// if (is_traversal_attempt(path))
	// {
		// generateErrorResponse(403);
		// std::cout << " hack attack " << std::endl;
		// return;
	// }
			// std::string local_path = build_local_path(server_conf.root, path);
			// int resource_status = check_resource(local_path);
			// std::cout << "Resource status for " << local_path << ": " << resource_status << std::endl;
			// if (mog.status != 200)
			// {
			// 	generateErrorResponse(mog.status);
			// 	return;
			// }
			if (mog.is_file)
			{
				std::ifstream file(mog.filesystem_path.c_str(), std::ios::in | std::ios::binary);
				if (!file.is_open())
				{
					if (errno == EACCES)
						*this = generateErrorResponse(403);
					else if (errno == ENOENT)
						*this = generateErrorResponse(404);
					else
						*this = generateErrorResponse(500);
					return;
				}
				std::stringstream buffer;
				buffer << file.rdbuf();
				this->setStatusCode(200);
				this->setReasonPhrase("OK");
				this->setBody(buffer.str());
				this->setHeader("Content-Type", get_mime_type(mog.filesystem_path));
				std::stringstream buuferLenght;
				buuferLenght << this->getBody().size();
				this->setHeader("Content-Length", buuferLenght.str());
				this->setHeader("Date", this->current_http_date());
				this->buildRawResponse();
				std::cout << this->getRawResponse() << std::endl;
			}
			else if (mog.is_redirect)
			{
					// if (path[mog.filesystem_path.size() - 1] != '/')
					// {
					// 	this->setStatusCode(301);
					// 	this->setReasonPhrase("Moved Permanently");
					// 	this->setHeader("Location", path + "/");
					// 	this->buildRawResponse();
					// 	std::cout << this->getRawResponse() << std::endl;
					// }
				}
}

void Response::dispatchMethod(const Request &req, const RouteResult &context)
{

	if (req.getMethod() == GET)
	{
		handleGet(req, context);
	}
	// else if (req.getMethod() == POST)
	// {
	// 	handlePost(req, context);
	// }
	// else if (req.getMethod() == DELETE)
	// {
	// 	handleDelete(context);
	// }
	// else
	// {
	// 	buildErrorResponse(501); // 501 Not Implemented
	// }
}

void Response::build(const Request &req, const RouteResult &context)
{

	// 1. Did the Router find a rule violation? (e.g., 405 Method Not Allowed)
	if (context.status != 200)
	{
		*this = generateErrorResponse(context.status);
		return;
	}

	// 2. Is this a Redirection?
	// if (context.is_redirect)
	// {
	// 	// We use matched_location here because that is where the redirect URL lives!
	// 	buildRedirectResponse(context.matched_location->redirect);
	// 	return;
	// }

	// // 3. Is it an Autoindex request?
	// if (context.is_autoindex)
	// {
	// 	buildDirectoryListing(context.filesystem_path);
	// 	return;
	// }

	// 4. If it is a normal file operation, pass it to the Dispatcher!
	if (context.is_file)
	{
		dispatchMethod(req, context);
	}
}

// void Response::build(const Request &req, const RouteContext mog)
// {
// 	if (req.getMethod() == GET)
// 	{
// 		if (std::find(mog.allow_methods.begin(), mog.allow_methods.end(), "GET") != mog.allow_methods.end())
// 			handleGet(req, mog);
// 		else
// 		{
// 			*this = generateErrorResponse(405);
// 			// std::cout << this->getRawResponse() << std::endl;
// 		}
// 	}
// 	// else if (req.getMethod() == POST)
// 	// 	handlePost(req);

// 	// else if (req.getMethod() == DELETE)
// 	// 	handleDelete(req);

// 	// else
// 	// 	generateErrorResponse(405);
// }
