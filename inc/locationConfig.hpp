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
    std::string              cgi_extension;   
		std::string              cgi_path;
    bool is_maxBody;
    std::string              upload_store;  
    std::string              redirect;         
    int                      return_code;        

    LocationConfig();
};

#endif 
