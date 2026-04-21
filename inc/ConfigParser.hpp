// ConfigParser.hpp
#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "ServerConfig.hpp"
// include about trim and split lines

class ConfigParser
{
    public:
        // Constructor — takes the config file path
        ConfigParser(const std::string& filename);

        // Everyone calls this to get all parsed servers
        std::vector<ServerConfig> getServers() const;

    private:
        std::string               _filename;
        std::vector<ServerConfig> _servers;

        // Main parsing function — called in constructor
        void parse();

        // Line helpers
        std::string              trim(const std::string& s);
        std::string              removeSemicolon(const std::string& s);
        std::vector<std::string> splitLine(const std::string& line, char delimiter);

        // Block parsers
        void parseServerLine(const std::string& key, const std::vector<std::string>& words, ServerConfig& server);

        void parseLocationLine(const std::string& key, const std::vector<std::string>& words, LocationConfig& location);
};

#endif