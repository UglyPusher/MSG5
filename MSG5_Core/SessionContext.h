#pragma once
#include <string>

class SessionContext {
public:
    explicit SessionContext(const std::string& token);

    const std::string& getToken() const;
    const std::string& getUserUid() const;
    const std::string& getRoleUid() const;
    const std::string& getShiftUid() const;

private:
    std::string sessionToken;
    std::string userUid = "mock-user";
    std::string roleUid = "mock-role";
    std::string shiftUid = "mock-shift";
};