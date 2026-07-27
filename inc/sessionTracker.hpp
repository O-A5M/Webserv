#ifndef SESSIONTRACKER_HPP
#define SESSIONTRACKER_HPP

#include <string>
#include <set>
#include <cstdlib> // For rand()
#include <ctime>   // For time()

class sessionTracker {
    private:
        std::set<std::string> _sessions;
        std::string generateSessionId();
    public:
        sessionTracker();
        ~sessionTracker();

        // Checks if the ID exists in our session set
        bool isValidSession(const std::string& sessionId);
        // Condition A: New User Logic
        std::string createSession();
        // Condition B: Returning User Logic
        void destroySession(const std::string& sessionId);
};

#endif