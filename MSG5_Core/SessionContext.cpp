#include "pch.h"
#include "SessionContext.h"

SessionContext::SessionContext(const std::string& token)
    : sessionToken(token)
{}

const std::string& SessionContext::getToken() const {
    return sessionToken;
}

const std::string& SessionContext::getUserUid() const {
    return userUid;
}

const std::string& SessionContext::getRoleUid() const {
    return roleUid;
}

const std::string& SessionContext::getShiftUid() const {
    return shiftUid;
}
