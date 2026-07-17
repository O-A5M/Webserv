#include "../../inc/Response.hpp"
#include "../../inc/sessionTracker.hpp"

static sessionTracker globalTracker; // about cookies

std::string Response::readHtmlTemplate(const std::string& filepath)
{
	std::ifstream file(filepath.c_str());
	if (!file.is_open()) 
        return "<h1>Error: Could not open cookies.html template on disk.</h1>";
	
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

void Response::replacePlaceholder(std::string& content, const std::string& placeholder, const std::string& replacement)
{
	size_t pos = content.find(placeholder);
	while (pos != std::string::npos) {
		content.replace(pos, placeholder.length(), replacement);
		pos = content.find(placeholder, pos + replacement.length());
	}
}

void Response::setCookie(const std::string &name, const std::string &value, const std::string &path, bool httpOnly) {
    std::string cookieStr = name + "=" + value;
    if (!path.empty()) cookieStr += "; Path=" + path;
    if (httpOnly) cookieStr += "; HttpOnly";
    
    setCookieHeaders.push_back(cookieStr);
}


void Response::handleVisitCounter(const Request &req, const RouteResult &context) {
    std::map<std::string, std::string> cookies = req.getCookies();
    std::string sessionId = cookies["session_id"];

    int visitCount = 0;
    bool isNewSession = false;

    if (req.getQuery() == "action=clear") {
    globalTracker.destroySession(sessionId);
    this->setHeader("Set-Cookie", "session_id=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/");
    this->setStatusCode(302);
    this->setReasonPhrase("Found");
    this->setHeader("Location", "/cookies");
    this->setHeader("Content-Length", "0");
    this->setHeader("Connection", "close");
    this->setBody("");
    this->buildRawResponse();
    return;
}

    if (globalTracker.isValidSession(sessionId)) {
        visitCount = globalTracker.incrementvisit(sessionId);
    } else {
        sessionId = globalTracker.createSession();
        visitCount = 1;
        isNewSession = true;
    }

    // 4. Read HTML from DISK and inject data
    std::string htmlBody = readHtmlTemplate(context.filesystem_path);
    
    // C++98 friendly int to string
    std::stringstream ss;
    ss << visitCount;
    
    replacePlaceholder(htmlBody, "{{VISIT_COUNT}}", ss.str());
    replacePlaceholder(htmlBody, "{{SESSION_ID}}", sessionId);

    // 5. Assemble headers and body
    this->setStatusCode(200);
    this->setReasonPhrase("OK");
    this->setHeader("Content-Type", "text/html");
    
    std::stringstream len_ss;
    len_ss << htmlBody.length();
    this->setHeader("Content-Length", len_ss.str());
    
    if (isNewSession) {
        this->setCookie("session_id", sessionId, "/", true);
    }

    this->setBody(htmlBody);
    this->buildRawResponse();
}

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

Response Response::generateErrorResponse(int code , const RouteResult &context)
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

	std::map<int, std::string>::const_iterator it = context.matched_server->error_pages.find(code);	
	if (it != context.matched_server->error_pages.end())
	{
		std::string custom_error_page_path = context.matched_server->root + "/" + it->second;
		if (fileExists(custom_error_page_path))
		{
			std::ifstream file(custom_error_page_path.c_str(), std::ios::in | std::ios::binary);
			if (file.is_open())
			{
				std::stringstream buffer;
				buffer << file.rdbuf();
				res.setStatusCode(code);
				res.setReasonPhrase(reason);
				res.setHeader("Content-Type", "text/html");
				std::ostringstream len;
				len << buffer.str().size();
				res.setHeader("Content-Length", len.str());
				res.setHeader("Connection", "close");
				res.setBody(buffer.str());
				res.buildRawResponse();
				return res;
			}
		}
	}
	
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

	// --- THIS PART IS VITAL ---
    // Inject the cookies you just set
	for (size_t i = 0; i < setCookieHeaders.size(); ++i) {
		response_stream << "Set-Cookie: " << setCookieHeaders[i] << "\r\n";
	}

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
			*this = generateErrorResponse(403 , context);
		else if (errno == ENOENT)
			*this = generateErrorResponse(404 , context	);
		else
			*this = generateErrorResponse(500, context);
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
}

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

		if (itemName == "." || itemName == "..")
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

	// 1. Method Check
	if (context.allow_methods.size() > 0 &&
		(std::find(context.allow_methods.begin(), context.allow_methods.end(), "POST") == context.allow_methods.end()))
	{
		*this = generateErrorResponse(405, context);
		return;
	}
	std::cout << "size------------------------: " << context.max_body_size << std::endl;
	if (context.matched_location != NULL && req.getBody().size() > context.max_body_size)
	{
		*this = generateErrorResponse(413, context);
		return;
	}
	std::string upload_dir;
	std::cout << "upload_store: " << context.matched_location->upload_store << std::endl;
	if (context.matched_location != NULL && !context.matched_location->upload_store.empty())
		upload_dir = context.matched_location->upload_store;
	else
		upload_dir = "./www/uploads";
	std::cout << "Upload directory: " << upload_dir << std::endl;
	struct stat st;
	if (stat(upload_dir.c_str(), &st) != 0)
	{
		if (mkdir(upload_dir.c_str(), 0755) != 0)
		{
			*this = generateErrorResponse(500	, context);
			return;
		}
	}
	else
	{
		if (!S_ISDIR(st.st_mode))
		{
			*this = generateErrorResponse(403	, context);
			return;
		}
	}
	if (access(upload_dir.c_str(), W_OK) != 0)
	{
		*this = generateErrorResponse(403	, context);
		return;
	}
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
			*this = generateErrorResponse(400, context);
			return;
		}
		std::ifstream temp_file(req.getBodyFilePath().c_str(), std::ios::binary);
		if (!temp_file.is_open())
		{
			*this = generateErrorResponse(500, context);
			return;
		}

		std::string search_boundary = boundary;
		char buffer[8192];
		std::string data_window = "";

		bool header_parsed = false;
		bool is_file = false;
		bool is_eof = false;

		std::ofstream out_file;

		while (true)
		{
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
							*this = generateErrorResponse(500, context);
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

	else
	{
		std::ifstream temp_file(req.getBodyFilePath().c_str(), std::ios::binary);
		if (!temp_file.is_open())
		{
			*this = generateErrorResponse(500, context);
			return;
		}

		std::string ext = ".bin";
		if (contentType.find("video/mp4") != std::string::npos)
			ext = ".mp4";
		else if (contentType.find("video/mpeg") != std::string::npos)
			ext = ".mpeg";
		else if (contentType.find("image/jpeg") != std::string::npos)
			ext = ".jpg";
		else if (contentType.find("image/png") != std::string::npos)
			ext = ".png";
		else if (contentType.find("application/octet-stream") != std::string::npos)
			ext = ".bin";

		std::stringstream unique_name;
		unique_name << "upload_" << std::time(NULL) << "_" << std::rand() << ext;
		std::string output_path = upload_dir + "/" + unique_name.str();

		std::ofstream out_file(output_path.c_str(), std::ios::binary);
		if (!out_file.is_open())
		{
			temp_file.close();
			*this = generateErrorResponse(500, context);
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

	this->setStatusCode(201);
	this->setReasonPhrase("Created");
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
		*this = generateErrorResponse(405	, context);
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
				*this = generateErrorResponse(500, context);
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
			*this = generateErrorResponse(403, context);
			return;
		}
	}
	else
	{
		*this = generateErrorResponse(403, context);
		return;
	}
}

void Response::handleDelete(const RouteResult &context)
{
	if (context.allow_methods.size() > 0 &&
		(std::find(context.allow_methods.begin(), context.allow_methods.end(), "DELETE") == context.allow_methods.end()))
	{
		*this = generateErrorResponse(405, context);
		return;
	}

	struct stat st;
	if (stat(context.filesystem_path.c_str(), &st) != 0)
	{
		*this = generateErrorResponse(404	, context);
		return;
	}

	if (S_ISDIR(st.st_mode))
	{
		*this = generateErrorResponse(403, context);
		return;
	}

	if (std::remove(context.filesystem_path.c_str()) != 0)
	{
		if (errno == EACCES || errno == EPERM)
		{
			*this = generateErrorResponse(403, context);
		}
		else
		{
			*this = generateErrorResponse(500, context);
		}
		return;
	}

	this->setStatusCode(200);
	this->setReasonPhrase("OK");

	std::string success_body = "<html><body><h1>Delete Successful</h1></body></html>";
	this->setBody(success_body);
	this->setHeader("Content-Type", "text/html");

	std::stringstream bufferLength;
	bufferLength << this->getBody().size();
	this->setHeader("Content-Length", bufferLength.str());
	this->setHeader("Date", this->current_http_date());

	this->buildRawResponse();
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
	else if (req.getMethod() == DELETE)
	{
		handleDelete(context);
	}
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

	if (context.is_redirect)
	{
		buildRedirectResponse(context);
		return;
	}
	
	// --- NEW: Trigger Phase 3 ---
    if (context.is_session_test)
    {
        handleVisitCounter(req, context);
        return;
    }

	if (context.status == 200)
	{
		dispatchMethod(req, context);
	}
	else
	{
		*this = generateErrorResponse(context.status, context);
	}
}
