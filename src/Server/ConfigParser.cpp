// PARSE CONFIGFILE TODO
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include <cstdlib>

ConfigParser::ConfigParser(const std::string& filename)
    : _filename(filename)
{
    parse();
}

// p2 and p3 call this to get data
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

// ─── Server Line Parser ─────────────────────────────────────
void ConfigParser::parseServerLine(const std::string& key,
                                   const std::vector<std::string>& words,
                                   ServerConfig& server)
{
    if (words[1].empty())
        throw std::runtime_error("Empty line in server block");
    if (key == "listen" && words.size() >= 2)
        server.port = std::atoi(removeSemicolon(words[1]).c_str());

    else if (key == "host" && words.size() >= 2)
        server.host = removeSemicolon(words[1]);

    else if (key == "server_name" && words.size() >= 2)
        server.server_name = removeSemicolon(words[1]);

    else if (key == "root" && words.size() >= 2)
        server.root = removeSemicolon(words[1]);

    else if (key == "index")
    {
        for (size_t i = 1; i < words.size(); i++)
            server.index.push_back(removeSemicolon(words[i]));
    }
    else if (key == "client_max_body_size" && words.size() >= 2)
        server.client_max_body_size = std::atoi(removeSemicolon(words[1]).c_str());

    else if (key == "error_page" && words.size() >= 3)
    {
        int code = std::atoi(words[1].c_str());
        server.error_pages[code] = removeSemicolon(words[2]);
    }
}

// ─── Location Line Parser ───────────────────────────────────
void ConfigParser::parseLocationLine(const std::string& key, const std::vector<std::string>& words, LocationConfig& location)
{
    if (key == "root" && words.size() >= 2)
        location.root = removeSemicolon(words[1]);

    else if (key == "index")
    {
        for (size_t i = 1; i < words.size(); i++)
            location.index.push_back(removeSemicolon(words[i]));
    }
    else if (key == "allow_methods" || key == "allowed_methods")
    {
        for (size_t i = 1; i < words.size(); i++)
            location.allow_methods.push_back(removeSemicolon(words[i]));
    }
    else if (key == "autoindex" && words.size() >= 2)
        location.autoindex = (removeSemicolon(words[1]) == "on");

    else if (key == "client_max_body_size" && words.size() >= 2)
        location.client_max_body_size = std::atoi(removeSemicolon(words[1]).c_str());

    else if (key == "cgi_extension" && words.size() >= 2)
        location.cgi_extension = removeSemicolon(words[1]);

    else if (key == "return" && words.size() >= 2)
        location.redirect = removeSemicolon(words[1]);
}

void ConfigParser::parse()
{
    // open file and read line by line
    std::ifstream infile(_filename.c_str());
    if (!infile)
        throw std::runtime_error("Failed to open config file: " + _filename);
    
    std::string    line;
    ServerConfig   currentServer;
    LocationConfig currentLocation;
    bool           inServer   = false;
    bool           inLocation = false;

    while (std::getline(infile, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        // Block server { ... }
        if (line.find("server {") == 0)
        {
            if (inServer)
                throw std::runtime_error("Nested server blocks are not allowed");
            inServer      = true;
            currentServer = ServerConfig();
            continue;
        }

        // Block location { ... }
        if (line.find("location") == 0 && line[line.size() - 1] == '{')
        {
            if (!inServer)
                throw std::runtime_error("Location block must be inside a server block");
            inLocation      = true;
            currentLocation = LocationConfig();

            std::string withoutKeyword = line.substr(9); // remove "location "
            size_t      bracePos       = withoutKeyword.rfind('{');
            currentLocation.path       = trim(withoutKeyword.substr(0, bracePos));
            continue;
        }

        // Unknown block
        if (!line.empty() && line[line.size() - 1] == '{')
            throw std::runtime_error("Unknown block type: " + line);

        // closing brace } for server or location
        if (line == "}")
        {
            if (inLocation)
            {
                currentServer.locations.push_back(currentLocation);
                inLocation = false;
            }
            else if (inServer)
            {
                _servers.push_back(currentServer);
                inServer = false;
            }
            continue;
        }

        // parse key-value lines
        std::vector<std::string> words = splitLine(line, ' ');
        if (words.empty())
            continue;

        std::string key = words[0];

        if (inLocation)
            parseLocationLine(key, words, currentLocation);
        else if (inServer)
            parseServerLine(key, words, currentServer);
    }

    // check if we ended while still inside a block
    if (inLocation || inServer)
        throw std::runtime_error("Config file ended before closing all blocks");
}