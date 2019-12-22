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

#include "Session.hpp"

#include <cstdint>
#include <string>
#include <list>
#include <mutex>

#include "UserAuth.hpp"
#include "Timestamp.hpp"

Session::Session(const int clientFd, const int sessionId)
    : m_clientFd{clientFd}
    , m_sessionId{sessionId}
{
    m_timestamp = Timestamp();
}

int Session::GetSessionId() {
    return m_sessionId;
}

int Session::GetClientFd() {
    return m_clientFd;
}

void Session::SetRequest(std::string& request) {
    m_request = request;
}

std::string Session::GetRequest() {
    return m_request;
}

void Session::SetHost(std::string& host) {
    m_hostUrl = host;
}

std::string Session::GetHost() {
    return m_hostUrl;
}

void Session::SetPort(std::uint16_t port) {
    m_port = port;
}

std::uint16_t Session::GetPort() {
    return m_port;
}

void Session::SetTransparent(const bool transparent) {
    m_transparent = transparent;
}

bool Session::GetTransparent() {
    return m_transparent;
}

void Session::SetClientAddress(const std::string& ipAddress) {
    m_clientIp = ipAddress;
}

const std::string& Session::GetClientAddress() {
    return m_clientIp;
}

void Session::SetUserName(const std::string& userName) {
    m_userName = userName;
}

const std::string& Session::GetUserName() {
    return m_userName;
}

void Session::AddRxBytes(size_t bytes) {
    m_clientRxBytes += bytes;
}

void Session::AddTxBytes(size_t bytes) {
    m_clientTxBytes += bytes;
}

std::uint64_t Session::GetRxBytes() {
    return m_clientRxBytes;
}

std::uint64_t Session::GetTxBytes() {
    return m_clientTxBytes;
}

std::uint64_t Session::GetTimestamp() {
    return m_timestamp;
}

SessionManager& SessionManager::Instance() {
    static SessionManager* instance = new SessionManager;
    return *instance;
}

Session* SessionManager::CreateSession(const int clientFd) {
    {
        auto lg = LockGuard{m_lock};

        m_sessionList.emplace_back(Session{clientFd, m_sessionId++});
    }
    return GetSessionForClient(clientFd);
}

Session* SessionManager::GetSessionForClient(const int clientFd) {
    auto lg = LockGuard{m_lock};

    for (auto it = m_sessionList.begin(); it != m_sessionList.end(); it++) {
        if (it->GetClientFd() == clientFd) {
            return &(*it);
        }
    }
    return nullptr;
}

Session* SessionManager::GetSession(const int sessionId) {
    auto lg = LockGuard{m_lock};

    for (auto it = m_sessionList.begin(); it != m_sessionList.end(); it++) {
        if (it->GetSessionId() == sessionId) {
            return &(*it);
        }
    }
    return nullptr;
}

void SessionManager::EndSession(const int sessionId) {
    auto lg = LockGuard{m_lock};

    for (auto it = m_sessionList.begin(); it != m_sessionList.end(); it++) {
        if (it->GetSessionId() == sessionId) {
            if (it->GetHost() != "") {
                // Only track user/session statistics if authorization + auditing are BOTH enabled
                if (AuthManager::Instance().IsEnabled() &&
                    AuthManager::Instance().IsAuditEnabled(it->GetUserName())) {
                    auto& stats = GlobalStats::Instance().GetStats(it->GetUserName(), it->GetHost());
                    stats.AddConnectionTime(Timestamp() - it->GetTimestamp());
                    stats.AddRxBytes(it->GetRxBytes());
                    stats.AddTxBytes(it->GetTxBytes());
                    stats.IncrementConnections();
                }
            }
            m_sessionList.erase(it);
            return;
        }
    }
}
