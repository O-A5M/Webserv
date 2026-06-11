#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <string>
#include <vector>


struct LocationConfig
{
    std::string              path;
    std::string              root;
    std::vector<std::string> index;
    std::vector<std::string> allow_methods;
    bool                     autoindex;
    size_t                   client_max_body_size;
    std::string              cgi_extension;        // p3
	std::string              cgi_path;        // p3
    std::string              upload_store;    // P3
    std::string              redirect;             // THIS (for return/redirect)
    int                     return_code;          // THIS (for return)

    LocationConfig();
};

#endif // LOCATION_CONFIG_HPP
