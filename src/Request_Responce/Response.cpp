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
	// =========================================================
	// TASK 1: PRE-FLIGHT CHECKS
	// =========================================================

	// 1. Method Check
	if (context.allow_methods.size() > 0 &&
			(std::find(context.allow_methods.begin(), context.allow_methods.end(), "POST") == context.allow_methods.end()))
	{
		*this = generateErrorResponse(405);
		return;
	}

	// 2. Size Limit Check (Safe against NULL location)
	// if (context.matched_location != NULL && req > context.matched_location->client_max_body_size)
	// {
	// 	*this = generateErrorResponse(413);
	// 	return;
	// }

	// 3. Path Resolution
	std::string upload_dir;
	if (context.matched_location != NULL && !context.matched_location->root.empty())
		upload_dir = context.matched_location->root;
	else if (context.matched_server != NULL && !context.matched_server->root.empty())
		upload_dir = context.matched_server->root;
	else
		upload_dir = ".";

	if (!upload_dir.empty() && upload_dir[upload_dir.size() - 1] == '/')
	{
		upload_dir.erase(upload_dir.size() - 1);
	}

	// 4. Directory Permissions
	struct stat st;
	if (stat(upload_dir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode) || access(upload_dir.c_str(), W_OK) != 0)
	{
		*this = generateErrorResponse(403);
		return;
	}

	// =========================================================
	// TASK 2: CONTENT-TYPE ROUTING
	// =========================================================

	std::string contentType = "";
	std::map<std::string, std::string>::const_iterator it = req.getHeaders().find("content-type");

	if (it != req.getHeaders().end())
	{
		contentType = it->second;
	}


	if (contentType.find("multipart/form-data") != std::string::npos)
	{
		std::string boundary = req.getBoundary();
		if (boundary.empty())
		{
			*this = generateErrorResponse(400);
			return;
		}
		std::ifstream temp_file(req.getBodyFilePath().c_str(), std::ios::binary);
		if (!temp_file.is_open())
		{
			*this = generateErrorResponse(500);
			return;
		}

		std::string search_boundary = boundary;
		char buffer[8192];
		std::string data_window = "";

		bool header_parsed = false;
		bool is_file = false; // "The Skipper" flag
		bool is_eof = false;	// EOF safety flag

		std::ofstream out_file;

		// Loop forever until we explicitly break
		while (true)
		{
			// 1. Scoop the data (only if we haven't hit EOF yet)
			if (!is_eof)
			{
				temp_file.read(buffer, sizeof(buffer));
				std::streamsize bytes_read = temp_file.gcount();
				if (bytes_read > 0)
				{
					data_window.append(buffer, bytes_read);
				}
				if (temp_file.eof() || bytes_read == 0)
				{
					is_eof = true;
				}
			}

			// 2. STATE A: Extract Headers & Filename
			if (!header_parsed)
			{
				size_t header_end = data_window.find("\r\n\r\n");
				if (header_end != std::string::npos)
				{
					std::string sub_headers = data_window.substr(0, header_end);

					size_t filename_pos = sub_headers.find("filename=\"");
					if (filename_pos != std::string::npos)
					{
						is_file = true;

						filename_pos += 10;
						size_t filename_end = sub_headers.find("\"", filename_pos);
						std::string safe_filename = "default.bin";

						if (filename_end != std::string::npos)
						{
							std::string raw_filename = sub_headers.substr(filename_pos, filename_end - filename_pos);
							safe_filename = "";
							for (size_t i = 0; i < raw_filename.size(); ++i)
							{
								char c = raw_filename[i];
								if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_')
									safe_filename += c;
								else
									safe_filename += '_';
							}
						}

						std::stringstream unique_name;
						unique_name << std::time(NULL) << "_" << std::rand() << "_" << safe_filename;
						std::string output_path = upload_dir + "/" + unique_name.str();

						out_file.open(output_path.c_str(), std::ios::binary);
						if (!out_file.is_open())
						{
							*this = generateErrorResponse(500);
							return;
						}
					}
					else
					{
						is_file = false;
					}

					data_window.erase(0, header_end + 4);
					header_parsed = true;
				}
				else
				{
					if (is_eof)
						break;
					continue;
				}
			}

			if (header_parsed)
			{
				size_t pos = data_window.find(search_boundary);

				if (pos != std::string::npos)
				{
					if (is_file && out_file.is_open())
					{
						out_file.write(data_window.data(), pos);
						out_file.close();
					}

					data_window.erase(0, pos + search_boundary.size());


					if (data_window.size() >= 2 && data_window.substr(0, 2) == "--")
					{
						break;
					}

					header_parsed = false;
				}
				else
				{
					if (data_window.size() > search_boundary.size())
					{
						size_t safe_to_write = data_window.size() - search_boundary.size();
						if (is_file && out_file.is_open())
						{
							out_file.write(data_window.data(), safe_to_write);
						}
						data_window.erase(0, safe_to_write);
					}
					else if (is_eof)
					{
						if (is_file && out_file.is_open())
						{
							out_file.write(data_window.data(), data_window.size());
						}
						break;
					}
				}
			}
		}

		if (out_file.is_open())
			out_file.close();
		temp_file.close();
	}

	else if (contentType.find("application/x-www-form-urlencoded") != std::string::npos ||
					 contentType.find("text/plain") != std::string::npos ||
					 contentType.find("application/json") != std::string::npos)
	{
		std::ifstream temp_file(req.getBodyFilePath().c_str(), std::ios::binary);
		if (!temp_file.is_open())
		{
			*this = generateErrorResponse(500);
			return;
		}

		std::stringstream unique_name;
		unique_name << "post_" << std::time(NULL) << "_" << std::rand() << ".txt";
		std::string output_path = upload_dir + "/" + unique_name.str();

		std::ofstream out_file(output_path.c_str(), std::ios::binary);
		if (!out_file.is_open())
		{
			*this = generateErrorResponse(500);
			return;
		}

		char buffer[8192];
		while (temp_file.read(buffer, sizeof(buffer)) || temp_file.gcount() > 0)
		{
			out_file.write(buffer, temp_file.gcount());
		}

		temp_file.close();
		out_file.close();
	}

	else
	{
		*this = generateErrorResponse(415); // Unsupported Media Type
		return;
	}

	this->setStatusCode(200);
	this->setReasonPhrase("OK");
	std::string success_body = "<html><body><h1>Upload Successful</h1></body></html>";
	this->setBody(success_body);
	this->setHeader("Content-Type", "text/html");
	std::stringstream buuferLenght;
	buuferLenght << this->getBody().size();
	this->setHeader("Content-Length", buuferLenght.str());
	this->setHeader("Date", this->current_http_date());
	this->buildRawResponse();
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
