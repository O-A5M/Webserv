// PARSE CONFIGFILE TODO
#include "ConfigParser.hpp"
#include "ServerConfig.hpp"
#include <cstdlib>

ConfigParser::ConfigParser(const std::string& filename)
: _filename(filename)
{
    parse();
}

static bool isCommentOrEmpty(const std::string& line)
{
    return line.empty() || line[0] == '#';
}

static size_t findNextMeaningfulLine(const std::vector<std::string>& lines, size_t start)
{
    while (start < lines.size() && isCommentOrEmpty(lines[start]))
        ++start;
    return start;
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
void ConfigParser::parseServerLine(const std::string& key, const std::vector<std::string>& words, ServerConfig& server)
{
    if (key == "listen" && words.size() >= 2)
    {
        // if (words.size() <= 1)
        //     throw std::runtime_error("listen directive should have only one argument");
        server.port = std::atoi(removeSemicolon(words[1]).c_str());
    }

    else if (key == "host" && words.size() >= 2)
    {
        // if (words.size() > 1)
        //     throw std::runtime_error("host directive should have only one argument");
        server.host = removeSemicolon(words[1]);
    }

    else if (key == "server_name" && words.size() >= 2)
    {
        // if (words.size() > 1)
        //     throw std::runtime_error("server_name directive should have only one argument");
        server.server_name = removeSemicolon(words[1]);
    }

    else if (key == "root" && words.size() >= 2)
    {
        // if (words.size() > 1)
        //     throw std::runtime_error("root directive should have only one argument");
        server.root = removeSemicolon(words[1]);
    }

    else if (key == "index")
    {
        for (size_t i = 1; i < words.size(); i++)
        {
            // if (words[i].empty() || words[i] == ";")
            //     throw::std::runtime_error("index directive must have at least one argument");
            server.index.push_back(removeSemicolon(words[i]));
        }
    }
    else if (key == "client_max_body_size" && words.size() >= 2)
    {
        // if (words.size() > 1)
        //     throw std::runtime_error("client_max_body_size directive should have only one argument");
        server.client_max_body_size = std::atoi(removeSemicolon(words[1]).c_str());
    }

    else if (key == "error_page" && words.size() >= 3)
    {
        // if (words.size() > 2)
        //     throw std::runtime_error("error_page directive should have exactly two arguments");
        int code = std::atoi(words[1].c_str());
        server.error_pages[code] = removeSemicolon(words[2]);
    }
}

// ─── Location Line Parser ───────────────────────────────────
void ConfigParser::parseLocationLine(const std::string& key, const std::vector<std::string>& words, LocationConfig& location)
{
    if (key == "root" && words.size() >= 2)
    {
        // if (words.size() > 1)
        //     throw std::runtime_error("root directive should have only one argument");
        location.root = removeSemicolon(words[1]);
    }

    else if (key == "index")
    {
        for (size_t i = 1; i < words.size(); i++)
        {
            // if (words[i].empty() || words[i] == ";")
            //     throw::std::runtime_error("index directive must have at least one argument");
            location.index.push_back(removeSemicolon(words[i]));
        }
    }
    else if (key == "allow_methods")
    {
        for (size_t i = 1; i < words.size(); i++)
        {
            // if (words[i].empty() || words[i] == ";")
            //     throw::std::runtime_error("allow_methods directive must have at least one argument");
            location.allow_methods.push_back(removeSemicolon(words[i]));
        }
    }
    else if (key == "autoindex" && words.size() >= 2)
    {
        // if (words.size() > 1)
        //     throw std::runtime_error("autoindex directive should have only one argument");
        location.autoindex = (removeSemicolon(words[1]) == "on");
    }

    else if (key == "client_max_body_size" && words.size() >= 2)
    {
        // if (words.size() > 1)
        //     throw std::runtime_error("client_max_body_size directive should have only one argument");
        location.client_max_body_size = std::atoi(removeSemicolon(words[1]).c_str());
    }

    else if (key == "cgi_extension" && words.size() >= 2)
    {
        // if (words.size() > 1)
        //     throw std::runtime_error("cgi_extension directive should have only one argument");
        location.cgi_extension = removeSemicolon(words[1]);
    }

    else if (key == "return" && words.size() >= 2)
    {
        // if (words.size() > 1)
        //     throw std::runtime_error("return directive should have only one argument");
        location.redirect = removeSemicolon(words[1]);
    }
}

void ConfigParser::parse()
{
    // open file and read line by line
    std::ifstream infile(_filename.c_str());
    if (!infile)
        throw std::runtime_error("Failed to open config file: " + _filename);
    
    std::vector<std::string> lines;
    std::string              line;

    while (std::getline(infile, line))
        lines.push_back(trim(line));

    ServerConfig   currentServer;
    LocationConfig currentLocation;
    bool           inServer   = false;
    bool           inLocation = false;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        line = lines[i];
        if (isCommentOrEmpty(line))
            continue;

        // Block server { ... }
        if (line.compare(0, 6, "server") == 0 && (line.size() == 6 || line[6] == ' ' || line[6] == '\t' || line[6] == '{'))
        {
            if (inServer)
                throw std::runtime_error("Nested server blocks are not allowed");

            std::string afterKeyword = trim(line.substr(6));
            if (afterKeyword.empty())
            {
                size_t nextLine = findNextMeaningfulLine(lines, i + 1);
                if (nextLine >= lines.size() || lines[nextLine] != "{")
                    throw std::runtime_error("Server block must be followed by braces");
                i = nextLine;
            }
            else if (afterKeyword != "{")
                throw std::runtime_error("Server block must be followed by braces");

            inServer      = true;
            currentServer = ServerConfig();
            continue;
        }

        // Block location { ... }
        if (line.compare(0, 8, "location") == 0 && (line.size() == 8 || line[8] == ' ' || line[8] == '\t' || line[8] == '{'))
        {
            if (!inServer)
                throw std::runtime_error("Location block must be inside a server block");

            std::string afterKeyword = trim(line.substr(8));
            std::string locationPath  = afterKeyword;

            size_t bracePos = afterKeyword.rfind('{');
            if (bracePos != std::string::npos)
                locationPath = trim(afterKeyword.substr(0, bracePos));
            else
            {
                size_t nextLine = findNextMeaningfulLine(lines, i + 1);
                if (nextLine >= lines.size() || lines[nextLine] != "{")
                    throw std::runtime_error("Location block must be followed by braces");
                i = nextLine;
            }

            if (locationPath.empty())
                throw std::runtime_error("Location block must declare a path");

            inLocation      = true;
            currentLocation = LocationConfig();
            currentLocation.path = locationPath;
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