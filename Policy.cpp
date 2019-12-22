/**
 *
 * NermalProxy
 *
 * Copyright 2019 Mark Slevinsky
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may
 * be used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
    return false;
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
