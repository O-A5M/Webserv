#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "serverConfig.hpp"
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

        // parse directive server block
        void parseDirectiveServer(const std::vector<std::string>& tokens, std::size_t &i, ServerConfig& server);
        void parseDirectiveListenS(const std::vector<std::string>& words, ServerConfig& server);
        void parseDirectiveServerNameS(const std::vector<std::string>& words, ServerConfig& server);
        void parseDirectiveRootS(const std::vector<std::string>& words, ServerConfig& server);
        void parseDirectiveIndexS(const std::vector<std::string>& words, ServerConfig& server);
        void parseDirectiveClientMaxBodySizeS(const std::vector<std::string>& words, ServerConfig& server);
        void parseDirectiveErrorPageS(const std::vector<std::string>& words, ServerConfig& server);

        // parse directive location block
        void parseDirectiveLocation(const std::vector<std::string>& tokens, std::size_t &i, LocationConfig& location);
        void parseDirectiveRootL(const std::vector<std::string>& words, LocationConfig& location);
        void parseDirectiveIndexL(const std::vector<std::string>& words, LocationConfig& location);
        void parseDirectiveClientMaxBodySizeL(const std::vector<std::string>& words, LocationConfig& location);
        void parseDirectiveAllowMethodsL(const std::vector<std::string>& words, LocationConfig& location);
        void parseDirectiveAutoIndexL(const std::vector<std::string>& words, LocationConfig& location);
        void parseDirectiveCgiExtensionL(const std::vector<std::string>& words, LocationConfig& location);
        void parseDirectiveCgiPathL(const std::vector<std::string>& words, LocationConfig& location);
        void parseDirectiveUploadStoreL(const std::vector<std::string>& words, LocationConfig& location);
        void parseDirectiveReturnRedirectL(const std::vector<std::string>& words, LocationConfig& location);

        // tokenize
        std::vector<std::string> tokenize();
        void tokenizeLine(const std::string& line, std::vector<std::string>& tokens);


};

#endif // CONFIGPARSER_HPP
