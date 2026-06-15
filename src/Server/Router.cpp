#include "Router.hpp"
#include <sys/stat.h>
#include <unistd.h>

Router::Router(const std::vector<ServerConfig>& servers)
    : servers(servers) {}




static bool apply_redirect_if_needed(const LocationConfig &location, RouteResult &result)
{
	if (location.redirect.empty())
		return false;

	result.is_redirect = true;
	result.status = location.return_code;
	result.redirect_location = location.redirect;

	if (location.return_code == 301)
		result.reason = "Moved Permanently";
	else if (location.return_code == 302)
		result.reason = "Found";
	else
		result.reason = "Redirect";

	return true;
}

RouteResult Router::route(const Request& req, int incoming_port) {
    RouteResult result;
    result.matched_server = NULL;
    result.matched_location = NULL;

    const ServerConfig* server = select_server(req, incoming_port);
    if (!server) {
        result.status = 500;
        result.reason = "No matching server found";
        return result;
    }
    result.matched_server = server;

    const LocationConfig* location = match_location(*server, req.getPath());
    if (!location) {
        result.status = 404;
        result.reason = "No matching location found";
        return result;
    }
    result.matched_location = location;

    result.allow_methods = location->allow_methods;

		if (apply_redirect_if_needed(*location, result))
			return result;

    result.filesystem_path = build_filesystem_path(*server, *location, req.getPath());

    validate_path(result.filesystem_path, *location, result);

    return result;
}



// const ServerConfig* Router::select_server(const Request& req, int incoming_port) {

const ServerConfig* Router::select_server(const Request& req, int incoming_port) {
    
    const std::map<std::string, std::string>& headers = req.getHeaders();
    std::map<std::string, std::string>::const_iterator host_it = headers.find("host");

    if (host_it == headers.end())
    {
        std::vector<ServerConfig>::const_iterator srv_it = servers.begin();
        for (; srv_it != servers.end(); ++srv_it) {
            if (srv_it->port == incoming_port) {
                return &(*srv_it);
            }
        }
        return NULL;
    }

    std::string host_header = host_it->second;

    std::vector<ServerConfig>::const_iterator srv_it = servers.begin();
    for (; srv_it != servers.end(); ++srv_it) {
        if (srv_it->server_name == host_header && srv_it->port == incoming_port) {
            return &(*srv_it);
        }
    }

    srv_it = servers.begin();
    for (; srv_it != servers.end(); ++srv_it) {
        if (srv_it->port == incoming_port) {
            return &(*srv_it);
        }
    }

    return NULL;
}

const LocationConfig* Router::match_location(const ServerConfig& server,
                                             const std::string& request_path) {
    const LocationConfig* best_match = NULL;

    std::vector<LocationConfig>::const_iterator loc_it = server.locations.begin();
    for (; loc_it != server.locations.end(); ++loc_it) {
        if (request_path.substr(0, loc_it->path.length()) == loc_it->path) {
            best_match = &(*loc_it);
        }
    }
    return best_match;
}

std::string Router::build_filesystem_path(const ServerConfig& server,
                                         const LocationConfig& location,
                                         const std::string& request_path) {

    std::string base_root;
    std::string filesystem_path;
    if (location.root.empty())
        base_root = server.root + "/";
    else
        base_root = location.root + "/";

		std::string path_after_location = request_path;
    std::string result = base_root + path_after_location;

    return result;
}

bool Router::validate_path(const std::string& path, const LocationConfig& location, RouteResult& result) {
    struct stat file_stat;

    if (stat(path.c_str(), &file_stat) != 0) {
        result.status = 404;
        result.reason = "File not found";
        return false;
    }
    if (S_ISREG(file_stat.st_mode)) {
        result.is_file = true;
        result.status = 200;
        result.reason = "File found";
        return true;
    }

    if (S_ISDIR(file_stat.st_mode)) {
        result.is_directory = true;
				result.physicalPath = path;

				std::vector<std::string> index_files = location.index;

				for (size_t i = 0; i < index_files.size(); ++i)
				{

					std::string index_path = path + "/" + index_files[i];
					result.filesystem_path = index_path;
					result.status = 200;
				}
        if (location.autoindex) {
            result.is_autoindex = true;
            result.status = 200;
            result.reason = "Autoindex enabled";
            return true;
        }
            if (location.autoindex) {
                result.is_autoindex = true;
                result.status = 200;
                result.reason = "Autoindex enabled";
                return true;
            }

        result.status = 200;
        result.reason = "Directory (no listing)";
        return false;
    }

    result.status = 403;
    result.reason = "Unknown file type";
    return false;
}
