// inc/RouteResult.hpp
#ifndef ROUTERESULT_HPP
#define ROUTERESULT_HPP

#include <string>
#include <vector>
#include <map>
#include "serverConfig.hpp"
#include "locationConfig.hpp"

// class LocationConfig; // forward declaration
// class ServerConfig;   // forward declaration

struct RouteResult {
    int status;                                    // 200, 301, 404, 403, 405, etc.
    std::string filesystem_path;                    // resolved absolute path
    const LocationConfig* matched_location;          // which location matched
    const ServerConfig* matched_server;             // which server matched
    bool is_cgi;                                     // CGI handler?
    bool is_autoindex;                               // directory listing?
    bool is_directory;                               // is a directory?
    bool is_file;                                    // is a file?
    bool is_redirect;                                // redirect response?
    std::string redirect_location;                   // where to redirect (301/302)
    std::vector<std::string> allow_methods;         // allowed methods (for 405)
    std::string cgi_script_path;                     // CGI script location
    std::map<std::string, std::string> cgi_env;      // CGI environment variables
    std::string error_page_path;                     // custom error page
    std::string reason;                             // human-readable reason
    
    // Constructor
    RouteResult();
};

#endif