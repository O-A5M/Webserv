#include "configParser.hpp"
#include <cstdlib>
#include <set>

ConfigParser::ConfigParser(const std::string &filename)
    : _filename(filename)
{
    parse();
}

std::vector<ServerConfig> ConfigParser::getServers() const
{
    return _servers;
}

static bool is_all_digits(const std::string &s)
{
    if (s.empty())
        return false;

    for (size_t i = 0; i < s.length(); ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;
    }

    return true;
}

static std::vector<std::string> collectDirectiveWords(const std::vector<std::string> &tokens, size_t &i)
{
    std::vector<std::string> words;
    while (i < tokens.size() && tokens[i] != ";")
        words.push_back(tokens[i++]);
    return words;
}

void ConfigParser::tokenizeLine(const std::string &line, std::vector<std::string> &tokens)
{
    std::string current;

    for (std::size_t i = 0; i < line.size(); i++)
    {
        char ch = line[i];

        if (ch == '#' && current.empty())
            break;

        if (ch == '{' || ch == '}' || ch == ';')
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
            tokens.push_back(std::string(1, ch));
        }
        else if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else
            current += ch;
    }

    if (!current.empty())
        tokens.push_back(current);
}

std::vector<std::string> ConfigParser::tokenize()
{
    std::ifstream infile(_filename.c_str());

    if (!infile)
        throw std::runtime_error("Failed to open config file: " + _filename);
    if (infile.peek() == std::ifstream::traits_type::eof())
        throw std::runtime_error("Config file is empty: " + _filename);

    std::vector<std::string> tokens;
    std::string line;

    while (std::getline(infile, line))
        tokenizeLine(line, tokens);
    if (tokens.empty())
        throw std::runtime_error("Config file contains only comments or whitespace: " + _filename);

    return tokens;
}

void ConfigParser::parseDirectiveListenS(const std::vector<std::string> &words, ServerConfig &server)
{
    if (words[1].find(':') != std::string::npos)
    {
        size_t colonPos = words[1].find(':');
        server.host = words[1].substr(0, colonPos);
        std::string portStr = words[1].substr(colonPos + 1);
        if (!is_all_digits(portStr))
            throw std::runtime_error("Invalid port value: " + portStr);
        int _port = std::atoi(portStr.c_str());
        if (_port <= 0 || _port > 65535)
            throw std::runtime_error("Invalid port number in listen directive: " + portStr);
        server.port = _port;
    }
    else
    {
        if (!is_all_digits(words[1]))
            throw std::runtime_error("Invalid port value: " + words[1]);
        int _port = std::atoi(words[1].c_str());
        if (_port <= 0 || _port > 65535)
            throw std::runtime_error("Invalid port number in listen directive: " + words[1]);
        server.port = _port;
    }
}

void ConfigParser::parseDirectiveServerNameS(const std::vector<std::string> &words, ServerConfig &server)
{
    server.server_name = words[1];
}

void ConfigParser::parseDirectiveUploadStoreL(const std::vector<std::string> &words, LocationConfig &location)
{
    location.upload_store = words[1];
}

void ConfigParser::parseDirectiveRootS(const std::vector<std::string> &words, ServerConfig &server)
{
    server.root = words[1];
}

void ConfigParser::parseDirectiveIndexS(const std::vector<std::string> &words, ServerConfig &server)
{
    for (size_t i = 1; i < words.size(); i++)
        server.index.push_back(words[i]);
}

void ConfigParser::parseDirectiveClientMaxBodySizeS(const std::vector<std::string> &words, ServerConfig &server)
{
    if (!is_all_digits(words[1]))
        throw std::runtime_error("Invalid client_max_body_size value: " + words[1]);
    server.client_max_body_size = static_cast<std::size_t>(std::atoi(words[1].c_str()));
    server.is_maxBody = true;
}

void ConfigParser::parseDirectiveErrorPageS(const std::vector<std::string> &words, ServerConfig &server)
{
    if (!is_all_digits(words[1]))
        throw std::runtime_error("Invalid error code in error_page directive: " + words[1]);
    int error_code = std::atoi(words[1].c_str());
    if (error_code < 400 || error_code > 599)
        throw std::runtime_error("Error code in error_page directive must be between 400 and 599: " + words[1]);
    server.error_pages[error_code] = words[2];
}

void ConfigParser::parseDirectiveServer(const std::vector<std::string> &tokens, std::size_t &i, ServerConfig &server)
{
    std::vector<std::string> words;
    if (tokens[i] == "listen")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 2)
            throw std::runtime_error("Too many arguments for listen directive: " + tokens[i]);
        if (words.size() < 2)
            throw std::runtime_error("Not enough arguments for listen directive: " + tokens[i]);

        parseDirectiveListenS(words, server);
        i++;
    }
    else if (tokens[i] == "server_name")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 2)
            throw std::runtime_error("Too many arguments for server_name directive: " + tokens[i]);
        if (words.size() < 2)
            throw std::runtime_error("Not enough arguments for server_name directive: " + tokens[i]);
        parseDirectiveServerNameS(words, server);
        i++;
    }
    else if (tokens[i] == "root")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 2)
            throw std::runtime_error("Too many arguments for root directive: " + tokens[i]);
        if (words.size() < 2)
            throw std::runtime_error("Not enough arguments for root directive: " + tokens[i]);
        parseDirectiveRootS(words, server);
        i++;
    }
    else if (tokens[i] == "index")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        parseDirectiveIndexS(words, server);
        i++;
    }
    else if (tokens[i] == "client_max_body_size")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 2)
            throw std::runtime_error("Too many arguments for client_max_body_size directive: " + tokens[i]);
        if (words.size() < 2)
            throw std::runtime_error("Not enough arguments for client_max_body_size directive: " + tokens[i]);
        parseDirectiveClientMaxBodySizeS(words, server);
        i++;
    }
    else if (tokens[i] == "error_page")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 3)
            throw std::runtime_error("Too many arguments for error_page directive: " + tokens[i]);
        if (words.size() < 3)
            throw std::runtime_error("Not enough arguments for error_page directive: " + tokens[i]);
        parseDirectiveErrorPageS(words, server);
        i++;
    }
    else
        throw std::runtime_error("Unknown directive in server block: " + tokens[i]);
}

void ConfigParser::parseDirectiveRootL(const std::vector<std::string> &words, LocationConfig &location)
{
    location.root = words[1];
}

void ConfigParser::parseDirectiveIndexL(const std::vector<std::string> &words, LocationConfig &location)
{
    for (size_t i = 1; i < words.size(); ++i)
        location.index.push_back(words[i]);
}

void ConfigParser::parseDirectiveClientMaxBodySizeL(const std::vector<std::string> &words, LocationConfig &location)
{
    if (!is_all_digits(words[1]))
        throw std::runtime_error("Invalid client_max_body_size value: " + words[1]);
    
    location.client_max_body_size = static_cast<std::size_t>(std::atoi(words[1].c_str()));
    location.is_maxBody = true;
}

void ConfigParser::parseDirectiveAllowMethodsL(const std::vector<std::string> &words, LocationConfig &location)
{
    for (size_t i = 1; i < words.size(); ++i)
    {
        if (words[i] != "GET" && words[i] != "POST" && words[i] != "DELETE")
            throw std::runtime_error("Invalid HTTP method in allow_methods directive: " + words[i]);
        location.allow_methods.push_back(words[i]);
    }
}

void ConfigParser::parseDirectiveAutoIndexL(const std::vector<std::string> &words, LocationConfig &location)
{
    if (words[1] == "on")
        location.autoindex = true;
    else if (words[1] == "off")
        location.autoindex = false;
    else
        throw std::runtime_error("Invalid autoindex value: " + words[1]);
}

void ConfigParser::parseDirectiveCgiExtensionL(const std::vector<std::string> &words, LocationConfig &location)
{
    std::vector<std::string> cgi_extensions;
    cgi_extensions.push_back(".php"); cgi_extensions.push_back(".pl"); cgi_extensions.push_back(".py"); cgi_extensions.push_back(".cgi");
    if (words[1].find('.') != std::string::npos)
    {
        size_t dotPos = words[1].find('.');
        std::string extension = words[1].substr(dotPos + 1);
        if (extension.empty())
            throw std::runtime_error("Invalid CGI extension: " + words[1]);
        for (size_t i = 0; i < cgi_extensions.size(); ++i)
        {
            if (words[1] == cgi_extensions[i])
            {
                location.cgi_extension = words[1];
                return;
            }
        }
        throw std::runtime_error("Invalid CGI extension: " + words[1]);
    }
    else
        throw std::runtime_error("Invalid missing dot: " + words[1]);
}

void ConfigParser::parseDirectiveCgiPathL(const std::vector<std::string> &words, LocationConfig &location)
{
    location.cgi_path = words[1];
}

void ConfigParser::parseDirectiveReturnRedirectL(const std::vector<std::string> &words, LocationConfig &location)
{
    int returnCode;
    if (words.size() != 3)
        throw std::runtime_error("Invalid return/redirect directive in location: " + words[0]);
    if (!is_all_digits(words[1]))
        throw std::runtime_error("Invalid return code in return/redirect directive: " + words[1]);
    returnCode = std::atoi(words[1].c_str());
    if (returnCode < 301 || returnCode > 308)
        throw std::runtime_error("Return code in return/redirect directive must be between 301 and 308: " + words[1]);
    location.return_code = returnCode;
    location.redirect = words[2];
}

void ConfigParser::parseDirectiveLocation(const std::vector<std::string> &tokens, std::size_t &i, LocationConfig &location)
{
    std::vector<std::string> words;
    if (tokens[i] == "root")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 2)
            throw std::runtime_error("Too many arguments for root directive: " + tokens[i]);
        if (words.size() < 2)
            throw std::runtime_error("Not enough arguments for root directive: " + tokens[i]);
        parseDirectiveRootL(words, location);
        i++;
    }
    else if (tokens[i] == "index")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        parseDirectiveIndexL(words, location);
        i++;
    }
    else if (tokens[i] == "client_max_body_size")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 2)
            throw std::runtime_error("Too many arguments for client_max_body_size directive: " + tokens[i]);
        if (words.size() < 2)
            throw std::runtime_error("Not enough arguments for client_max_body_size directive: " + tokens[i]);
        parseDirectiveClientMaxBodySizeL(words, location);
        i++;
    }
    else if (tokens[i] == "allow_methods")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 4)
            throw std::runtime_error("Too many arguments for allow_methods directive: " + tokens[i]);
        if (words.size() < 2)
            throw std::runtime_error("Not enough arguments for allow_methods directive: " + tokens[i]);
        parseDirectiveAllowMethodsL(words, location);
        i++;
    }
    else if (tokens[i] == "autoindex")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 2)
            throw std::runtime_error("Too many arguments for autoindex directive: " + tokens[i]);
        if (words.size() < 2)
            throw std::runtime_error("Not enough arguments for autoindex directive: " + tokens[i]);
        parseDirectiveAutoIndexL(words, location);
        i++;
    }
    else if (tokens[i] == "cgi_extension")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 2)
            throw std::runtime_error("Too many arguments for cgi_extension directive: " + tokens[i]);
        if (words.size() < 2)
            throw std::runtime_error("Not enough arguments for cgi_extension directive: " + tokens[i]);
        parseDirectiveCgiExtensionL(words, location);
        i++;
    }
    else if (tokens[i] == "cgi_path")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 2)
            throw std::runtime_error("Too many arguments for cgi_path directive: " + tokens[i]);
        if (words.size() < 2)
            throw std::runtime_error("Not enough arguments for cgi_path directive: " + tokens[i]);
        parseDirectiveCgiPathL(words, location);
        i++;
    }
    else if (tokens[i] == "upload_store")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 2)
            throw std::runtime_error("Too many arguments for upload_store directive: " + tokens[i]);
        if (words.size() < 2)
            throw std::runtime_error("Not enough arguments for upload_store directive: " + tokens[i]);
        parseDirectiveUploadStoreL(words, location);
        i++;
    }
    else if (tokens[i] == "return" || tokens[i] == "redirect")
    {
        words.clear();
        words = collectDirectiveWords(tokens, i);
        if (words.size() > 3)
            throw std::runtime_error("Too many arguments for return/redirect directive: " + tokens[i]);
        if (words.size() < 3)
            throw std::runtime_error("Not enough arguments for return/redirect directive: " + tokens[i]);
        parseDirectiveReturnRedirectL(words, location);
        i++;
    }
    else
        throw std::runtime_error("Unknown directive in location block: " + tokens[i]);
}

void ConfigParser::parse()
{
    std::vector<std::string> tokens = tokenize();
    std::size_t i = 0;

    // main loop
    while (i < tokens.size())
    {
        std::vector<std::string> words;
        if (tokens[i] != "server")
            throw std::runtime_error("Expected 'server' directive: " + tokens[i]);
        i++;
        if (tokens[i] != "{")
            throw std::runtime_error("Expected '{' after 'server': " + tokens[i]);
        i++;
        ServerConfig server;
        while (i < tokens.size() && tokens[i] != "}")
        {
            if (tokens[i] == "location")
            {
                LocationConfig location;
                i++;
                location.path = tokens[i++];
                if (location.path[0] != '/')
                    throw std::runtime_error("Invalid location path doesn't start with /: " + location.path);
                if (i >= tokens.size() || tokens[i] != "{")
                    throw std::runtime_error("Expected '{' after 'location " + location.path + "'");
                i++;
                while (i < tokens.size() && tokens[i] != "}")
                {
                    if (tokens[i] == "listen" || tokens[i] == "server_name")
                        throw std::runtime_error("Server-level directive not allowed in location block: " + tokens[i]);
                    if (tokens[i] == "location")
                        throw std::runtime_error("Nested location blocks are not allowed: " + tokens[i]);
                    parseDirectiveLocation(tokens, i, location);
                }
                server.locations.push_back(location);
                i++;
            }
            else
                parseDirectiveServer(tokens, i, server);
        }
        if (tokens[i] != "}")
            throw std::runtime_error("Expected '}' at the end of server block: " + tokens[i]);
        _servers.push_back(server);
        i++;
    }
}

