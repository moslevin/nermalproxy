#pragma once

#include <string>
#include <string.h>
#include <list>
#include <vector>
#include "Log.hpp"

class User {
public:
    User(const std::string& name, const std::string& password);
    void AddIp(const std::string& ipAddress);
    void SetAudit(bool audit);

    const std::string& GetName() const;
    const std::string& GetPassword() const;
    bool HasIpList() const;
    std::vector<std::string> GetIpList() const;

private:
    std::vector<std::string> m_ipList;
    std::string m_name;
    std::string m_password;
    bool m_audit;
};

class AuthManager {
public:
    static AuthManager& Instance();

    void SetEnabled(bool enable);
    bool IsEnabled();

    bool IsAuditEnabled(const std::string& user);
    void SetAudit(const std::string& user, bool audit);
    void AddUser(const std::string& user, const std::string& password);
    void AddUserIp(const std::string& user, const std::string& ip);
    bool AuthenticateIp(const std::string& ipAddress, std::string& username);
    bool Authenticate(const std::string& base64Creds, std::string& username);

private:

    bool Authenticate_i(const std::string& userName, const std::string& password);
    static std::uint8_t* Base64Decode(const std::string& source, int& size);

    std::list<User> m_users;
    bool m_enabled = false;
};

class SiteStats {
public:
    SiteStats(const std::string& hostName);

    const std::string& GetHostName();

    void AddRxBytes(std::uint64_t rxBytes);
    void AddTxBytes(std::uint64_t txBytes);
    void IncrementConnections();
    void AddConnectionTime(std::uint64_t timeMs);
    std::uint64_t GetRxBytes() const;
    std::uint64_t GetTxBytes() const;
    std::uint64_t GetTotalTime() const;
    void Print() const;
    void LogToFile(int fd) const;

private:
    std::string     m_hostName;
    std::uint64_t   m_rxBytes;
    std::uint64_t   m_txBytes;
    std::uint64_t   m_totalTime;
    int             m_connections;
};

class UserStats {
public:
    UserStats(const std::string& userName);

    SiteStats& GetStats(const std::string& host);
    const std::string& GetUserName();
    void LogToFile();
    int GetStatCount();

private:
    std::string m_userName;
    std::list<SiteStats> m_stats;
};

class GlobalStats {
public:
    GlobalStats();
    static GlobalStats& Instance();

    SiteStats& GetStats(const std::string& username, const std::string& host);
    void DumpUserStats(const std::string& username);
    void LogAndReset();

    bool ReadyToLog();

private:
    static constexpr auto m_logFrequency = 3600 * 24; // seconds
    static constexpr auto m_logPersistence = 30;

    std::list<UserStats> m_userStats;
    int m_logIndex;
    std::uint64_t m_lastLogTime;
};
