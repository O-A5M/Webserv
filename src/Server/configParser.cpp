#include "configParser.hpp"
#include <cstdlib>


ConfigParser::ConfigParser(const std::string& filename)
: _filename(filename)
{
    parse();
}

std::vector<ServerConfig> ConfigParser::getServers() const
{
    return _servers;
}

std::string ConfigParser::trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");

    if (start == std::string::npos || end == std::string::npos)
        return ""; // string is all whitespace

    return s.substr(start, end - start + 1);
}

std::string ConfigParser::removeSemicolon(const std::string& s)
{
    if (!s.empty() && s[s.size() - 1] == ';')
        return s.substr(0, s.size() - 1);
    return s;
}

std::vector<std::string> ConfigParser::splitLine(const std::string& line, char delimiter)
{
    std::vector<std::string> words;
    std::istringstream       iss(line);
    std::string              word;
    
    while (std::getline(iss, word, delimiter)) {
        if (!word.empty()) {
            words.push_back(word);
        }
    }
    return words;
}

static bool startsServerDirective(const std::string& token)
{
    return token == "listen"
        || token == "server_name"
        || token == "root"
        || token == "index"
        || token == "client_max_body_size"
        || token == "error_page"; 
}

static bool startsLocationDirective(const std::string& token)
{
    return token == "root"
        || token == "index"
        || token == "allow_methods"
        || token == "autoindex"
        || token == "client_max_body_size"
        || token == "cgi_extension"
        || token == "cgi_path"
        || token == "upload_store"
        || token == "return"
        || token == "redirect";
}

void ConfigParser::parseServerLine(const std::string& key, const std::vector<std::string>& words, ServerConfig& server)
{
    if (key == "listen") {
        if (words.size() != 2)
            throw std::runtime_error("Invalid listen directive: " + key);
        int _port = std::atoi(words[1].c_str());
        if (_port <= 0 || _port > 65535)
            throw std::runtime_error("Invalid port number in listen directive: " + words[1]);
        server.port = _port;
    }

    else if (key == "server_name") {
        if (words.size() != 2)
            throw std::runtime_error("Invalid server_name directive: " + key);
        server.server_name = words[1];
    }

    else if (key == "root") {
        if (words.size() != 2)
            throw std::runtime_error("Invalid root directive: " + key);
        server.root = words[1];
    }

    else if (key == "index") {
        for (size_t i = 1; i < words.size(); i++) {
            server.index.push_back(words[i]);
        }
    }

    else if (key == "client_max_body_size") {
        if (words.size() != 2)
            throw std::runtime_error("Invalid client_max_body_size directive: " + key);
        server.client_max_body_size = static_cast<std::size_t>(std::strtoul(words[1].c_str(), NULL, 10));
        if (!isdigit(words[1][0]))
            throw std::runtime_error("Invalid client_max_body_size value: " + words[1]);
    }

    else if (key == "error_page") {
        if (words.size() != 3)
            throw std::runtime_error("Invalid error_page directive: " + key);
        int error_code = std::atoi(words[1].c_str());
        if (error_code < 100 || error_code > 599)
            throw std::runtime_error("Invalid error code in error_page directive: " + words[1]);
        else if (!isdigit(words[1][0]))
            throw std::runtime_error("Invalid error code in error_page directive: " + words[1]);
        server.error_pages[error_code] = words[2];
    }

    else {
        throw std::runtime_error("Unknown directive in server block: " + key);
    }
}

void ConfigParser::parseLocationLine(const std::string& key, const std::vector<std::string>& words, LocationConfig& location)
{
    if (key == "root") {
        if (words.size() != 2)
            throw std::runtime_error("Invalid root directive in location: " + key);
        location.root = words[1];
    }
    else if (key == "index") {
        for (size_t i = 1; i < words.size(); ++i) {
            location.index.push_back(words[i]);
        }
    }
    else if (key == "allow_methods") {
        for (size_t i = 1; i < words.size(); ++i) {
            location.allow_methods.push_back(words[i]);
        }
        // display allowed methods for debugging
        // std::cout << "Parsed allow_methods for location " << location.path << ": ";
        for (size_t i = 0; i < location.allow_methods.size(); ++i) {
            std::cout << "========>" << location.allow_methods[i] << "\n";
        }
        // std::cout << std::endl;
    }
    else if (key == "autoindex") {
        if (words.size() != 2)
            throw std::runtime_error("Invalid autoindex directive in location: " + key);
        location.autoindex = (words[1] == "on"); 
    }
    else if (key == "client_max_body_size") {
        if (words.size() != 2)
            throw std::runtime_error("Invalid client_max_body_size directive: " + key);
        location.client_max_body_size = static_cast<std::size_t>(std::strtoul(words[1].c_str(), NULL, 10));
        if (!isdigit(words[1][0]))
            throw std::runtime_error("Invalid client_max_body_size value: " + words[1]);
    }
    else if (key == "cgi_extension") {
        if (words.size() != 2)
            throw std::runtime_error("Invalid cgi_extension directive in location: " + key);
        location.cgi_extension = words[1];
    }
    else if (key == "cgi_path") {
        if (words.size() != 2)
            throw std::runtime_error("Invalid cgi_path directive in location: " + key);
        location.cgi_path = words[1];
    }
    else if (key == "upload_store") {
        if (words.size() != 2)
            throw std::runtime_error("Invalid upload_store directive in location: " + key);
        location.upload_store = words[1];
    }
    else if (key == "return" || key == "redirect") {
        if (words.size() != 3)
            throw std::runtime_error("Invalid return/redirect directive in location: " + key);
        int return_code = std::atoi(words[1].c_str());
        if (return_code < 100 || return_code > 599)
            throw std::runtime_error("Invalid return code in return/redirect directive: " + words[1]);
        else if (!isdigit(words[1][0]))
            throw std::runtime_error("Invalid return code in return/redirect directive: " + words[1]);
        location.return_code = return_code;
        location.redirect = words[2];
    }
    else {
        throw std::runtime_error("Unknown directive in server block: " + key);
    }
}

std::vector<std::string> ConfigParser::tokenize()
{
    std::ifstream infile(_filename.c_str());
    if (!infile)
        throw std::runtime_error("Failed to open config file: " + _filename);

    std::vector<std::string> tokens;
    std::string              line;

    while (std::getline(infile, line)) {
        std::string current;

        for (std::size_t i = 0; i < line.size(); i++) {
            char ch = line[i];

            if (ch == '#' && current.empty())
                break;

            if (ch == '{' || ch == '}' || ch == ';') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                tokens.push_back(std::string(1, ch));
            }
            else if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
            }
            else {
                current += ch;
            }
        }

        if (!current.empty())
            tokens.push_back(current);
    }

    return tokens;
}

void ConfigParser::parse()
{
    std::vector<std::string> tokens = tokenize();
    std::size_t i = 0;

    while (i < tokens.size())
    {
        if (tokens[i] != "server")
            throw std::runtime_error("Expected 'server'");

        ServerConfig server;
        i++;

        if (i >= tokens.size() || tokens[i] != "{")
            throw std::runtime_error("Expected '{' after 'server'");
        i++;

        while (i < tokens.size() && tokens[i] != "}")
        {
            if (tokens[i] == "location")
            {
                LocationConfig location;
                std::vector<std::string> words;

                i++;
                if (i >= tokens.size()) // 
                    throw std::runtime_error("Unexpected end of file after 'location'");

                location.path = tokens[i++];

                if (i >= tokens.size() || tokens[i] != "{")
                    throw std::runtime_error("Expected '{' after 'location " + location.path + "'");
                i++;

                while (i < tokens.size() && tokens[i] != "}")
                {
                    words.clear();
                    while (i < tokens.size() && tokens[i] != ";" && tokens[i] != "}")
                        words.push_back(tokens[i++]);

                    for (size_t j = 1; j < words.size(); ++j) {
                        if (startsLocationDirective(words[j]))
                            throw std::runtime_error("Expected ';' after location directive");
                    }

                    if (words.empty())
                        throw std::runtime_error("Empty directive in location block");

                    parseLocationLine(words[0], words, location);
                    i++;
                }

                if (i >= tokens.size() || tokens[i] != "}")
                    throw std::runtime_error("Expected '}' to close location block for path: " + location.path);
                i++;

                server.locations.push_back(location);
            }
            else
            {
                std::vector<std::string> words;

                while (i < tokens.size() && tokens[i] != ";" && tokens[i] != "}")
                    words.push_back(tokens[i++]);
                for (size_t j = 1; j < words.size(); j++) {
                    if (startsServerDirective(words[j]))
                        throw std::runtime_error("Expected ';' after server directive");
                }
                parseServerLine(words[0], words, server);
                i++; // about to skip ';' or '}'
            }
        }

        if (i >= tokens.size() || tokens[i] != "}")
            throw std::runtime_error("Expected '}' to close server block");
        ++i;
        _servers.push_back(server);
        // if (tokens[i] == "server")
        //     std::cout << "another server" << std::endl;
    }
}