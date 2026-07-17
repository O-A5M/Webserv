// inc/RouteResult.hpp

#include "RouteResult.hpp"

RouteResult::RouteResult()
	: status(0)
	, filesystem_path()
	, matched_location(NULL)
	, matched_server(NULL)
	, is_cgi(false)
	, is_autoindex(false)
	, is_directory(false)
	, max_body_size(0)
	, is_file(false)
	, is_session_test(false)
	, is_redirect(false)
	, redirect_location()
	, allow_methods()
	, cgi_script_path()
	, cgi_env()
	, cgi_extension()
	, error_page_path()
	, reason()
{
}