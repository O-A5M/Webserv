#include "../../inc/Response.hpp"
#include "../../inc/sessionTracker.hpp"
#include "../../inc/RouteResult.hpp"
#include "../../inc/Request.hpp"

static sessionTracker globalTracker; // about cookies

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

// Helper function to trim spaces
// static std::string trimSpace(const std::string& str) {
//     size_t first = str.find_first_not_of(' ');
//     if (std::string::npos == first) return "";
//     size_t last = str.find_last_not_of(' ');
//     return str.substr(first, (last - first + 1));
// }

// Phase 1 Tokenizer
// static std::map<std::string, std::string> parseCookies(const std::string& cookieHeader) {
//     std::map<std::string, std::string> cookies;
//     size_t start = 0, end = 0;

//     while ((end = cookieHeader.find(';', start)) != std::string::npos) {
//         std::string pair = cookieHeader.substr(start, end - start);
//         size_t eq_pos = pair.find('=');
//         if (eq_pos != std::string::npos) {
//             cookies[trimSpace(pair.substr(0, eq_pos))] = trimSpace(pair.substr(eq_pos + 1));
//         }
//         start = end + 1;
//     }
//     std::string last_pair = cookieHeader.substr(start);
//     size_t eq_pos = last_pair.find('=');
//     if (eq_pos != std::string::npos) {
//         cookies[trimSpace(last_pair.substr(0, eq_pos))] = trimSpace(last_pair.substr(eq_pos + 1));
//     }
//     return cookies;
// }

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
        this->setHeader("Location", "/cookies");
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

void Response::buildRawResponse() {
    std::stringstream response_stream;
    
    // Status Line
    response_stream << "HTTP/1.1 " << this->status_code << " " << this->reason_phrase << "\r\n";
    
    // Standard Headers
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
        response_stream << it->first << ": " << it->second << "\r\n";
    
    // --- THIS PART IS VITAL ---
    // Inject the cookies you just set
    for (size_t i = 0; i < setCookieHeaders.size(); ++i)
        response_stream << "Set-Cookie: " << setCookieHeaders[i] << "\r\n";
    
    response_stream << "\r\n" << this->body;
    
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

	// --- NEW: Trigger Phase 3 ---
    if (context.is_session_test)
    {
        handleVisitCounter(req, context);
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
