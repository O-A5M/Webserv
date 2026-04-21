#include "LocationConfig.hpp"

LocationConfig::LocationConfig()
    : path(""),
      root(""),
      autoindex(false),
      client_max_body_size(1000000),
      cgi_extension(""),
      cgi_path(""),
      upload_store(""),
      redirect("")
{
}
