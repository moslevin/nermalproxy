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

#pragma once

#include <string>
#include <string.h>
#include <list>
#include <vector>
#include <ctime>

#include "Log.hpp"
#include "TimeMap.hpp"

/**
 * @brief The User class represents a user and its associated
 * policy, including IP addresses, auth credentials, and any
 * time-based access policies.
 */
class User {
public:
    User(const std::string& name, const std::string& password);
    void AddIp(const std::string& ipAddress);
    void SetAudit(bool audit);

    const std::string& GetName() const;
    const std::string& GetPassword() const;
    bool GetAudit() const;
    bool HasIpList() const;
    std::vector<std::string> GetIpList() const;
    WeeklyAccess& GetWeeklyAccess();

private:
    std::vector<std::string> m_ipList;
    std::string m_name;
    std::string m_password;
    WeeklyAccess m_weeklyAccess;
    bool m_audit;
};

/**
 * @brief The AuthManager class manages user authentication and
 * policy application, based on user/host/time/other parameters.
 */
class AuthManager {
public:
    static AuthManager& Instance();

    void SetEnabled(bool enable);
    bool IsEnabled();

    bool IsAuditEnabled(const std::string& username);
    void SetAudit(const std::string& username, bool audit);
    void AddUser(const std::string& username, const std::string& password);
    void AddUserIp(const std::string& username, const std::string& ip);
    bool AuthenticateIp(const std::string& ipAddress, std::string& username);
    bool Authenticate(const std::string& base64Creds, std::string& username);
    bool AccessAllowedAtTime(const std::string& username);
    bool SetWeeklyAccess(const std::string& user, const std::string& day, const std::string& initString);

private:

    bool Authenticate_i(const std::string& userName, const std::string& password);
    static std::uint8_t* Base64Decode(const std::string& source, int& size);

    std::list<User> m_users;
    bool m_enabled = false;
};

/**
 * @brief The SiteStats class tracks access statistics for a
 * given host.
 */
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

/**
 * @brief The UserStats class tracks site access statistics for
 * a host, used for implementing user-level auditing.
 */
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

/**
 * @brief The GlobalStats class tracks audit information for all users
 */
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
