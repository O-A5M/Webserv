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
// 	if (stat(local_path.c_str(),fi &le_info) != 0)
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

bool fileExists(const std::string &path)
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;
	return S_ISREG(st.st_mode);
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

void Response::serveFile(const RouteResult &context)
{
	std::ifstream file(context.filesystem_path.c_str(), std::ios::in | std::ios::binary);
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
	this->setHeader("Content-Type", get_mime_type(context.filesystem_path));
	std::stringstream buuferLenght;
	buuferLenght << this->getBody().size();
	this->setHeader("Content-Length", buuferLenght.str());
	this->setHeader("Date", this->current_http_date());
	this->buildRawResponse();
	std::cout << this->getRawResponse() << std::endl;
}

#include <dirent.h>
#include <sys/stat.h>
#include <string>

std::string Response::buildAutoIndex(const std::string &physicalPath, const std::string &requestURI)
{
	std::string html = "<html><head><title>Index of " + requestURI + "</title></head><body>";
	html += "<h1>Index of " + requestURI + "</h1><hr><ul>";

	DIR *dir = opendir(physicalPath.c_str());
	if (dir == NULL)
	{
		return "";
	}
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{

		std::string itemName = entry->d_name;

		if (itemName == ".")
		{
			continue;
		}
		std::string fullItemPath = physicalPath + "/" + itemName;
		struct stat st;

		if (stat(fullItemPath.c_str(), &st) == 0)
		{
			if (S_ISDIR(st.st_mode))
			{
				itemName += "/";
			}
		}
		std::string href;

		if (requestURI[requestURI.length() - 1] == '/')
		{
			href = requestURI + itemName;
		}
		else
		{
			href = requestURI + "/" + itemName;
		}

		html += "<li><a href=\"" + href + "\">" + itemName + "</a></li>";
	}

	closedir(dir);
	html += "</ul><hr></body></html>";

	return html;
}

void Response::handlePost(const Request &req, const RouteResult &context)
{
	std::map<std::string, std::string>::const_iterator it = req.getHeaders().find("content-type");
	if (it != req.getHeaders().end())
	{
		std::string contentType = it->second;
		if (contentType.find("multipart/form-data") != std::string::npos)
		{
			std::ifstream temp_body_file(req.getTempFilePath().c_str(), std::ios::binary);
			if (!temp_body_file.is_open())
			{
				this->buildErrorResponse(500);
				return;
			}
		}
	}
}

void Response::handleGet(const Request &req, const RouteResult &context)
{

	(void)req;
	if (context.allow_methods.size() > 0 && (std::find(context.allow_methods.begin(), context.allow_methods.end(), "GET") == context.allow_methods.end()))
	{
		*this = generateErrorResponse(405);
		return;
	}
	if (context.is_file)
	{
		serveFile(context);
	}
	else if (context.is_directory)
	{
		if (fileExists(context.filesystem_path))
		{
			serveFile(context);
			return;
		}
		else if (context.is_autoindex)
		{
			std::string autoIndexContent = buildAutoIndex(context.physicalPath, req.getUri());
			if (autoIndexContent.empty())
			{
				*this = generateErrorResponse(500);
				return;
			}
			this->setStatusCode(200);
			this->setReasonPhrase("OK");
			this->setBody(autoIndexContent);
			this->setHeader("Content-Type", "text/html");
			std::stringstream buuferLenght;
			buuferLenght << this->getBody().size();
			this->setHeader("Content-Length", buuferLenght.str());
			this->setHeader("Date", this->current_http_date());
			this->buildRawResponse();
		}
		else
		{
			*this = generateErrorResponse(403);
			return;
		}
	}
	else
	{
		*this = generateErrorResponse(403);
		return;
	}
}

void Response::dispatchMethod(const Request &req, const RouteResult &context)
{

	if (req.getMethod() == GET)
	{
		handleGet(req, context);
	}
	else if (req.getMethod() == POST)
	{
		handlePost(req, context);
	}
	// else if (req.getMethod() == DELETE)
	// {
	// 	handleDelete(context);
	// }
	// else
	// {
	// 	buildErrorResponse(501); // 501 Not Implemented
	// }
}

void Response::buildRedirectResponse(const RouteResult &context)
{
	this->setStatusCode(context.status);
	this->setReasonPhrase("Moved Permanently");
	this->setHeader("Location", context.redirect_location);
	this->setHeader("Date", this->current_http_date());
	this->setHeader("Content-Length", "0");
	this->setHeader("Connection", "keep-alive");
	this->setHeader("Server", "Webserv/1.0 (Ubuntu)");
	this->buildRawResponse();
}
void Response::build(const Request &req, const RouteResult &context)
{

	// 1. Did the Router find a rule violation? (e.g., 405 Method Not Allowed)
	// if (context.status != 200)
	// {
	// 	*this = generateErrorResponse(context.status);
	// 	return;
	// }
	if (context.is_redirect)
	{
		buildRedirectResponse(context);
		return;
	}
	// 4. If it is a normal file operation, pass it to the Dispatcher!
	if (context.status == 200)
	{
		dispatchMethod(req, context);
	}
	else
	{
		*this = generateErrorResponse(context.status);
	}
}

// void Response::build(const Request &req, const RouteContext context)
// {
// 	if (req.getMethod() == GET)
// 	{
// 		if (std::find(context.allow_methods.begin(), context.allow_methods.end(), "GET") != context.allow_methods.end())
// 			handleGet(req, context);
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
