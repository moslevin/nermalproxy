#pragma once

#include <cstdint>
#include <string>
#include <list>
#include <mutex>

#include "UserAuth.hpp"
#include "Timestamp.hpp"

class Session {
public:
    Session(const int clientFd, const int sessionId)
        : m_clientFd{clientFd}
        , m_sessionId{sessionId}
    {
        m_timestamp = Timestamp();
    }

    int GetSessionId() {
        return m_sessionId;
    }

    int GetClientFd() {
        return m_clientFd;
    }

    void SetRequest(std::string& request) {
        m_request = request;
    }

    std::string GetRequest() {
        return m_request;
    }

    void SetHost(std::string& host) {
        m_hostUrl = host;
    }

    std::string GetHost() {
        return m_hostUrl;
    }

    void SetPort(std::uint16_t port) {
        m_port = port;
    }

    std::uint16_t GetPort() {
        return m_port;
    }

    void SetTransparent(const bool transparent) {
        m_transparent = transparent;
    }

    bool GetTransparent() {
        return m_transparent;
    }

    void SetClientAddress(const std::string& ipAddress) {
        m_clientIp = ipAddress;
    }

    const std::string& GetClientAddress() {
        return m_clientIp;
    }

    void SetUserName(const std::string& userName) {
        m_userName = userName;
    }

    const std::string& GetUserName() {
        return m_userName;
    }

    void AddRxBytes(size_t bytes) {
        m_clientRxBytes += bytes;
    }

    void AddTxBytes(size_t bytes) {
        m_clientTxBytes += bytes;
    }

    std::uint64_t GetRxBytes() {
        return m_clientRxBytes;
    }

    std::uint64_t GetTxBytes() {
        return m_clientTxBytes;
    }

    std::uint64_t GetTimestamp() {
        return m_timestamp;
    }

private:
    int m_sessionId;
    int m_clientFd;
    int m_proxyFd;
    bool m_transparent;

    std::uint64_t m_clientRxBytes;
    std::uint64_t m_clientTxBytes;

    std::string m_hostUrl;
    std::uint16_t m_port;

    std::string m_request;
    std::string m_clientIp;

    std::string m_userName;
    std::uint64_t m_timestamp;
};

class SessionManager {
public:
    static SessionManager& Instance() {
        static SessionManager* instance = new SessionManager;
        return *instance;
    }

    Session* CreateSession(const int clientFd) {
        {
            auto lg = LockGuard{m_lock};

            m_sessionList.emplace_back(Session{clientFd, m_sessionId++});
        }
        return GetSessionForClient(clientFd);
    }

    Session* GetSessionForClient(const int clientFd) {
        auto lg = LockGuard{m_lock};

        for (auto it = m_sessionList.begin(); it != m_sessionList.end(); it++) {
            if (it->GetClientFd() == clientFd) {
                return &(*it);
            }
        }
        return nullptr;
    }

    Session* GetSession(const int sessionId) {
        auto lg = LockGuard{m_lock};

        for (auto it = m_sessionList.begin(); it != m_sessionList.end(); it++) {
            if (it->GetSessionId() == sessionId) {
                return &(*it);
            }
        }
        return nullptr;
    }

    void EndSession(const int sessionId) {
        auto lg = LockGuard{m_lock};

        for (auto it = m_sessionList.begin(); it != m_sessionList.end(); it++) {
            if (it->GetSessionId() == sessionId) {
                if (it->GetHost() != "") {
                    // Add stats to global stats table.
                    auto& stats = GlobalStats::Instance().GetStats(it->GetUserName(), it->GetHost());
                    stats.AddConnectionTime(Timestamp() - it->GetTimestamp());
                    stats.AddRxBytes(it->GetRxBytes());
                    stats.AddTxBytes(it->GetTxBytes());
                    stats.IncrementConnections();
                    m_sessionList.erase(it);
                }
                return;
            }
        }
    }

private:
    using LockGuard = std::unique_lock<std::mutex>;
    std::mutex m_lock;
    int m_sessionId = 0;
    std::list<Session> m_sessionList;
};
