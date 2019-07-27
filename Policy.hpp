#pragma once

#include <list>
#include <memory>

class UserPolicy {
public:
    UserPolicy(const std::string& name)
        : m_name{name}
    {}

    const std::string& GetName() {
        return m_name;
    }

    void AddDomain(const std::string& domain) {
        m_domains.emplace_back(domain);
    }

    bool GetDomain(int idx, std::string& domain) {
        if (idx >= m_domains.size()) {
            return false;
        }
        auto i = 0;
        for (auto it = m_domains.begin(); it != m_domains.end(); it++, i++) {
            if (i == idx) {
                domain = *it;
                return true;
            }
        }
    }

private:
    std::string m_name;
    std::list<std::string> m_domains;
};

class UserPolicyList {
public:
    void AddPolicy(std::unique_ptr<UserPolicy> policy) {
        m_policies.emplace_back(std::move(policy));
    }

    static UserPolicyList& Instance() {
        static UserPolicyList* instance = new UserPolicyList;
        return *instance;
    }

    bool GetUserDomain(int idx, const std::string& user, std::string &domain) {
        for (auto it = m_policies.begin(); it != m_policies.end(); it++) {
            if (it->get()->GetName() == user) {
                return it->get()->GetDomain(idx, domain);
            }
        }
        return false;
    }

private:
    std::list<std::unique_ptr<UserPolicy>> m_policies;
};
