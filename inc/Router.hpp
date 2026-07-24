// inc/Router.hpp
#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "RouteResult.hpp"
#include <vector>
#include <string>
#include "serverConfig.hpp"
#include "locationConfig.hpp"
#include "Request.hpp"

// class Request;
// class ServerConfig;
// class LocationConfig;

class Router {
public:
    Router(const std::vector<ServerConfig>& servers);
    RouteResult route(const Request& req, int incoming_port);

private:
    const ServerConfig* select_server(const Request& req, int incoming_port);
    
    const LocationConfig* match_location(const ServerConfig& server, 
                                         const std::string& request_path);
    
    std::string build_filesystem_path(RouteResult &res, const ServerConfig& server,
                                     const LocationConfig& location,
                                     const std::string& request_path);
    
    bool validate_path(const std::string& path, const ServerConfig& server,  const LocationConfig& location, RouteResult& result);
    
    const std::vector<ServerConfig>& servers; 
};

#endif