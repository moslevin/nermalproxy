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
