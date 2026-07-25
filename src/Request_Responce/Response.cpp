#include "../../inc/Response.hpp"
#include "../../inc/sessionTracker.hpp"

static sessionTracker globalTracker; // about cookies

void Response::manageGlobalSession(const Request &req)
{
    std::map<std::string, std::string> cookies = req.getCookies();
    std::string sessionId = cookies["session_id"];

    if (globalTracker.isValidSession(sessionId))
        this->currentSessionId = sessionId;
    else
    {
        // new session
        this->currentSessionId = globalTracker.createSession();
        // send cookies to browser bach l mera jaya ybe9a nefes l ID dima 
        this->setCookie("session_id", this->currentSessionId, "/", true);
    }
}

void Response::setCookie(const std::string &name, const std::string &value, const std::string &path, bool httpOnly) {
    std::string cookieStr = name + "=" + value;
    if (!path.empty()) cookieStr += "; Path=" + path;
    if (httpOnly) cookieStr += "; HttpOnly";
    
    setCookieHeaders.push_back(cookieStr);
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
	case 411:
		reason = "Length Required";
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
	if (context.matched_server)
	{
	
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

void Response::serveFile(const RouteResult &context , const Request &req)
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
	if (req.con == 0)
		this->setHeader("Connection", "close");
	else
		this->setHeader("Connection", "keep-alive");
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
	if (context.allow_methods.size() > 0 &&
		(std::find(context.allow_methods.begin(), context.allow_methods.end(), "POST") == context.allow_methods.end()))
	{
		std::cout << "Method POST not allowed for this location." << std::endl;
		*this = generateErrorResponse(405, context);
		return;
	}
	std::string upload_dir;
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
		std::string filename;
        size_t last_slash = req.getPath().find_last_of('/');
        if (last_slash != std::string::npos)
            filename = req.getPath().substr(last_slash + 1);
        else
            filename = req.getPath();
        //Fallback to a generated name if the URL ended with a slash
        if (filename.empty())
        {
            std::stringstream unique_name;
            unique_name << "upload_" << std::time(NULL) << "_" << std::rand() << ext;
            filename = unique_name.str();
        }
        std::string output_path = upload_dir + "/" + filename;
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
	if (req.con == 0)
		this->setHeader("Connection", "close");
	else
		this->setHeader("Connection", "keep-alive");
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
		serveFile(context , req);
	}
	else if (context.is_directory)
	{
		if (fileExists(context.filesystem_path))
		{
			serveFile(context , req);
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
			if (req.con == 0)
				this->setHeader("Connection", "close");
			else
				this->setHeader("Connection", "keep-alive");
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

void Response::handleDelete(const RouteResult &context , const Request &req)
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
	if (req.con == 0)
		this->setHeader("Connection", "close");
	else
		this->setHeader("Connection", "keep-alive");
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
		handleDelete(context, req);
	}
	// else
	// {
	// 	buildErrorResponse(501); // 501 Not Implemented
	// }
}

void Response::buildRedirectResponse(const RouteResult &context , const Request &req)
{
	this->setStatusCode(context.status);
	this->setReasonPhrase("Moved Permanently");
	this->setHeader("Location", context.redirect_location);
	this->setHeader("Date", this->current_http_date());
	this->setHeader("Content-Length", "0");
	if (req.con == 0)
		this->setHeader("Connection", "close");
	else
		this->setHeader("Connection", "keep-alive");
	this->setHeader("Server", "Webserv/1.0 (Ubuntu)");
	this->buildRawResponse();
}
void Response::build(const Request &req, const RouteResult &context)
{
	manageGlobalSession(req);

	if (context.is_redirect)
	{
		buildRedirectResponse(context , req);
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

void Response::buildFromCgi(const std::string &cgiOutput, const RouteResult &context)
{
	if (cgiOutput.empty())
	{
		*this = generateErrorResponse(502, context);
		return;
	}

	size_t headerEnd = cgiOutput.find("\r\n\r\n");
	size_t sepLen = 4;
	if (headerEnd == std::string::npos)
	{
		headerEnd = cgiOutput.find("\n\n");
		sepLen = 2;
	}

	std::string headerBlock;
	std::string cgiBody;
	if (headerEnd == std::string::npos)
    {
        *this = generateErrorResponse(502, context);
        return;
    }
    headerBlock = cgiOutput.substr(0, headerEnd);
    cgiBody = cgiOutput.substr(headerEnd + sepLen);
	int statusCode = 200;
	std::string reasonPhrase = "OK";
	bool haveContentType = false;

	std::stringstream headerStream(headerBlock);
	std::string line;
	while (std::getline(headerStream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			continue;

		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;

		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

		size_t start = value.find_first_not_of(" \t");
		value = (start == std::string::npos) ? "" : value.substr(start);

		std::string lowerKey = key;
		for (size_t i = 0; i < lowerKey.size(); ++i)
			lowerKey[i] = std::tolower(static_cast<unsigned char>(lowerKey[i]));

		if (lowerKey == "status")
		{
			std::stringstream ss(value);
			ss >> statusCode;
			size_t sp = value.find(' ');
			if (sp != std::string::npos)
				reasonPhrase = value.substr(sp + 1);
		}
		else
		{
			if (lowerKey == "content-type")
				haveContentType = true;
			this->setHeader(key, value);
		}
	}

	if (!haveContentType)
		this->setHeader("Content-Type", "text/html");

	this->setStatusCode(statusCode);
	this->setReasonPhrase(reasonPhrase);
	this->setBody(cgiBody);

	std::stringstream len;
	len << cgiBody.size();
	this->setHeader("Content-Length", len.str());
	this->setHeader("Date", this->current_http_date());

	this->buildRawResponse();
}