#include "sessionTracker.hpp"
#include <iostream>

sessionTracker::sessionTracker() {
    std::srand(std::time(0));
}

sessionTracker::~sessionTracker() {}

std::string sessionTracker::generateSessionId()
{
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const int lenght = 16;
    std::string result;

    for (int i = 0; i < lenght; ++i) {
        result += charset[std::rand() % (sizeof(charset) - 1)];
    }
    return result;
}

bool sessionTracker::isValidSession(const std::string& sessionId)
{
    if (sessionId.empty())
        return false;
    return _sessions.find(sessionId) != _sessions.end();
}

std::string sessionTracker::createSession()
{
    std::string newId = generateSessionId();

    while (isValidSession(newId)){
        newId = generateSessionId();
    }

    _sessions.insert(newId);
    return newId;
}

void sessionTracker::destroySession(const std::string& sessionId) {
    if (isValidSession(sessionId)) {
        _sessions.erase(sessionId);
    }
}