#include "Router.hpp"
#include <sys/stat.h>
#include <unistd.h>

Router::Router(const std::vector<ServerConfig>& servers)
    : servers(servers) {}


// static std::string methodToString(e_Methodes method) {
//     switch (method) {
//         case GET:     return "GET";
//         case POST:    return "POST";
//         case DELETE:  return "DELETE";
//         default:      return "UNKNOWN";
//     }
// }

// static bool is_regular_file(const std::string& path) {
//     struct stat st;
//     if (stat(path.c_str(), &st) != 0)
//         return false;
//     return S_ISREG(st.st_mode);
// }

static bool apply_redirect_if_needed(const LocationConfig& location, RouteResult& result)
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

// MAIN METHOD - called from your code
RouteResult Router::route(const Request& req, int incoming_port) {
    RouteResult result;
    result.matched_server = NULL;
    result.matched_location = NULL;
    
    // Step 1: Select which server applies
    const ServerConfig* server = select_server(req, incoming_port);
    if (!server) {
        result.status = 500;
        result.reason = "No matching server found";
        return result;
    }
    result.matched_server = server;
    
    // Step 2: Select which location applies
    const LocationConfig* location = match_location(*server, req.getPath());
    if (!location) {
        result.status = 404;
        result.reason = "No matching location found";
        return result;
    }
    result.matched_location = location;
    
    // Step 3: Check if method is allowed
    result.allow_methods = location->allow_methods;
    // std::string request_method = methodToString(req.getMethod());

    // bool method_allowed = false;
    // for (std::vector<std::string>::const_iterator it = result.allow_methods.begin();
    //     it != result.allow_methods.end(); ++it) {
    //         if (*it == request_method) {
    //         method_allowed = true;
    //         break;
    //     }
    // }
    
    // if (!method_allowed) {
    //     result.status = 405;
    //     result.reason = "Method not allowed: " + request_method;
    //     return result;
    // }

    // check if redirect applies before building filesystem path
    if (apply_redirect_if_needed(*location, result))
        return result;

    // Step 4: Build the actual filesystem path
    result.filesystem_path = build_filesystem_path(*server, *location, req.getPath());
    // Step 5: Validate the path exists and is accessible
    validate_path(result.filesystem_path, *location, result);
    
    return result;
}


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
    
    std::string host_header = host_it->second; // e.g., "api.example.com:8080"
    
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
    // size_t longest_prefix = 0;
    
    // Check all locations and find longest prefix match
    std::vector<LocationConfig>::const_iterator loc_it = server.locations.begin();
    for (; loc_it != server.locations.end(); ++loc_it) {
        // Does this location's path match the request path?
        if (request_path.substr(0, loc_it->path.length()) == loc_it->path) {
            // This location matches
            // if (loc_it->path.length() > longest_prefix) {
            //     longest_prefix = loc_it->path.length();
            // }
            best_match = &(*loc_it);
        }
    }
    return best_match;
}


// Step 3: Convert URL path to actual filesystem path
std::string Router::build_filesystem_path(const ServerConfig& server,
                                         const LocationConfig& location,
                                         const std::string& request_path) {
    // fills autoindex.
    std::string base_root;
    std::string filesystem_path;
    if (location.root.empty())
        base_root = server.root + "/";
    else
        base_root = location.root + "/";
    filesystem_path = base_root + request_path;
    return filesystem_path;
}

// Step 4: Check if path exists, is accessible, etc.
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
        
        std::vector<std::string> index_files = location.index;
        
        for (size_t i = 0; i < index_files.size(); ++i) {
            
            std::string index_path = path + "/" + index_files[i];
            // www/index/index.html
            // return true;
            // std::cout << "Checking for index file: " << index_path << std::endl;
        
            // if (is_regular_file(index_path)) {
            //     std::cout << "DKHEEEEL" << std::endl;
            //     result.filesystem_path = index_path;
            //     result.is_directory = false;
            //     result.is_file = true;
            //     result.status = 200;
            //     result.reason = "Index file found: " + index_files[i];
            //     return true;
            // }
            // else 
            result.filesystem_path = index_path;
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
