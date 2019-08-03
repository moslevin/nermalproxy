#pragma once

#include <list>
#include <memory>

class UserPolicy {
public:
    UserPolicy(const std::string& name);

    const std::string& GetName();
    void AddDomain(const std::string& domain);
    bool GetDomain(int idx, std::string& domain);

private:
    std::string m_name;
    std::list<std::string> m_domains;
};

class UserPolicyList {
public:
    void AddPolicy(std::unique_ptr<UserPolicy> policy);
    static UserPolicyList& Instance();
    bool GetUserDomain(int idx, const std::string& user, std::string &domain);

private:
    std::list<std::unique_ptr<UserPolicy>> m_policies;
};
