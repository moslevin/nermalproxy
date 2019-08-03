#pragma once

#include <cstdint>
#include <string>
#include <list>
#include <mutex>

#include "UserAuth.hpp"
#include "Timestamp.hpp"

class Session {
public:
    Session(const int clientFd, const int sessionId);
    int GetSessionId();
    int GetClientFd();

    void SetRequest(std::string& request);
    std::string GetRequest();
    void SetHost(std::string& host);

    std::string GetHost();

    void SetPort(std::uint16_t port);

    std::uint16_t GetPort();

    void SetTransparent(const bool transparent);

    bool GetTransparent();

    void SetClientAddress(const std::string& ipAddress);

    const std::string& GetClientAddress();
    void SetUserName(const std::string& userName);

    const std::string& GetUserName();

    void AddRxBytes(size_t bytes);

    void AddTxBytes(size_t bytes);

    std::uint64_t GetRxBytes();

    std::uint64_t GetTxBytes();

    std::uint64_t GetTimestamp();

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
    static SessionManager& Instance();

    Session* CreateSession(const int clientFd);

    Session* GetSessionForClient(const int clientFd);

    Session* GetSession(const int sessionId);

    void EndSession(const int sessionId);

private:
    using LockGuard = std::unique_lock<std::mutex>;
    std::mutex m_lock;
    int m_sessionId = 0;
    std::list<Session> m_sessionList;
};
