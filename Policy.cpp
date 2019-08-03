#include "Policy.hpp"

#include <list>
#include <memory>

UserPolicy::UserPolicy(const std::string& name)
    : m_name{name}
{}

const std::string& UserPolicy::GetName() {
    return m_name;
}

void UserPolicy::AddDomain(const std::string& domain) {
    m_domains.emplace_back(domain);
}

bool UserPolicy::GetDomain(int idx, std::string& domain) {
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

void UserPolicyList::AddPolicy(std::unique_ptr<UserPolicy> policy) {
    m_policies.emplace_back(std::move(policy));
}

UserPolicyList& UserPolicyList::Instance() {
    static UserPolicyList* instance = new UserPolicyList;
    return *instance;
}

bool UserPolicyList::GetUserDomain(int idx, const std::string& user, std::string &domain) {
    for (auto it = m_policies.begin(); it != m_policies.end(); it++) {
        if (it->get()->GetName() == user) {
            return it->get()->GetDomain(idx, domain);
        }
    }
    return false;
}
