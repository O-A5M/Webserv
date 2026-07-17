#ifndef SESSIONTRACKER_HPP
#define SESSIONTRACKER_HPP

#include <string>
#include <map>
#include <cstdlib> // For rand()
#include <ctime>   // For time()

class sessionTracker {
    private:
        std::map<std::string, int> _visits;
        std::string generateSessionId();
    public:
        sessionTracker();
        ~sessionTracker();

        // Checks if the ID exists in our map
        bool isValidSession(const std::string& sessionId);
        // Condition A: New User Logic
        std::string createSession();
        // Condition B: Returning User Logic
        int incrementvisit(const std::string& sessionId);
        void destroySession(const std::string& sessionId);
};

#endif