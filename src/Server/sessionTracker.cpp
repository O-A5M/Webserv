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
    return _visits.find(sessionId) != _visits.end();
}

std::string sessionTracker::createSession()
{
    std::string newId = generateSessionId();

    while (isValidSession(newId)){
        newId = generateSessionId();
    }

    _visits[newId] = 1;
    return newId;
}

int sessionTracker::incrementvisit(const std::string& sessionId)
{
    _visits[sessionId] += 1;
    return _visits[sessionId];
}

void sessionTracker::destroySession(const std::string& sessionId) {
    if (isValidSession(sessionId)) {
        _visits.erase(sessionId);
    }
}