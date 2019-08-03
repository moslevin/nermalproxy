#include "UserAuth.hpp"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <string.h>
#include <list>
#include <vector>

#include "Log.hpp"
#include "Timestamp.hpp"

namespace {
int s_logNumber = 0;

} // anonymous namespace

User::User(const std::string& name, const std::string& password)
    : m_name{std::move(name)}
    , m_password{std::move(password)}
    , m_ipList{}
    , m_audit{false}
{}

void User::AddIp(const std::string& ipAddress) {
    m_ipList.emplace_back(ipAddress);
}

void User::SetAudit(bool audit) {
    m_audit = audit;
}

const std::string& User::GetName() const {
    return m_name;
}

const std::string& User::GetPassword() const {
    return m_password;
}

bool User::HasIpList() const {
    if (m_ipList.size() != 0) {
        return true;
    }
    return false;
}

std::vector<std::string> User::GetIpList() const {
    return m_ipList;
}

AuthManager& AuthManager::Instance() {
    static AuthManager* instance = new AuthManager;
    return *instance;
}

void AuthManager::SetEnabled(bool enable) {
    m_enabled = enable;
}

bool AuthManager::IsEnabled() {
    return m_enabled;
}

bool AuthManager::IsAuditEnabled(const std::string &username) {

}

void AuthManager::SetAudit(const std::string& username, bool audit)
{
    for (auto& user: m_users) {
        if (user.GetName() == username) {
            user.SetAudit(audit);
        }
    }
}
void AuthManager::AddUser(const std::string& user, const std::string& password) {
    m_users.emplace_back(User{user, password});
}

void AuthManager::AddUserIp(const std::string& username, const std::string& ipAddress) {
    for (auto& user: m_users) {
        if (user.GetName() == username) {
            user.AddIp(ipAddress);
        }
    }
}

bool AuthManager::AuthenticateIp(const std::string& ipAddress, std::string& username) {
    Log(LogSeverity::Debug, "%s: enter", __func__);
    for (auto& user: m_users) {
        Log(LogSeverity::Debug, "%s: User=%s", __func__, user.GetName().c_str());
        if (user.HasIpList()) {
            Log(LogSeverity::Debug, "%s: ... has an IP list", __func__);
            auto ipList = user.GetIpList();
            for (auto& ip : ipList) {
                Log(LogSeverity::Debug, "%s: Check IP=%s", __func__, ip.c_str());
                if (ip == ipAddress) {
                    Log(LogSeverity::Debug, "%s: Match -- auth by IP Ok", __func__, ip.c_str());
                    username = user.GetName();
                    return true;
                }
            }
        } else {
            Log(LogSeverity::Debug, "%s: ... has no IP list", __func__);
        }
    }
    return false;
}

bool AuthManager::Authenticate(const std::string& base64Creds, std::string& username) {
    auto size = int{};
    auto* credString = Base64Decode(base64Creds, size);
    if (credString) {
        char user[128] = {};
        char pass[128] = {};

        auto delimPtr = strchr((char*)credString, ':');
        if (delimPtr) {
            *delimPtr = ' ';
        }

        auto parsed = sscanf((const char*)credString, "%s %s", user, pass);
        auto userString = std::string(user);
        auto passString = std::string(pass);

        auto rc = Authenticate_i(userString, passString);
        delete credString;

        if (rc) {
            username = userString;
        }
        return rc;
    }
    return false;
}

bool AuthManager::Authenticate_i(const std::string& userName, const std::string& password) {
    Log(LogSeverity::Debug, "%s:  %s:%s", __func__, userName.c_str(), password.c_str());
    for (auto& user : m_users) {
        Log(LogSeverity::Debug, "%s:  %s:%s", __func__, user.GetName().c_str(), user.GetPassword().c_str());
        if ((user.GetName() == userName) && (user.GetPassword() == password)) {
            Log(LogSeverity::Debug, "Credentials validated");
            return true;
        }
    }
    Log(LogSeverity::Warn, "Credential Error");
    return false;
}

std::uint8_t* AuthManager::Base64Decode(const std::string& source, int& size) {

    size = ((source.size() * 3) + 3) / 4;
    auto* rc = new uint8_t[size + 3];
    memset(rc, 0, size + 3);

    auto* dst = rc;
    auto idx = 0;
    auto tmp = std::uint32_t{};

    while (source[idx]) {
        auto in = source[idx++];

        //! Note - if we wanted to do base64->binary, we'd also need to check
        //! for 2 == chars at the end and adjust the size, but for text-only
        //! this is okay.
        if (in != '=') {
            auto c = std::uint32_t{};

            if (in >= 'A' && in <= 'Z') {
                c = in - 'A';
            } else if (in >= 'a' && in <= 'z') {
                c = (in - 'a') + 26;
            } else if (in >= '0' && in <= '9') {
                c = (in - '0') + 52;
            } else if (in == '+') {
                c = 62;
            } else if (in == '/') {
                c = 63;
            } else {
                Log(LogSeverity::Debug, "Invalid Base64 Character");
                delete rc;
                return nullptr;
            }

            tmp |= (c & 0x3F);
        }

        if ((idx % 4) == 0) {
            *dst++ = std::uint8_t(tmp >> 16);
            *dst++ = std::uint8_t(tmp >> 8);
            *dst++ = std::uint8_t(tmp & 0xFF);
            tmp = 0;
        } else {
            tmp <<= 6;
        }
    }

    if ((idx % 4) != 0) {
        Log(LogSeverity::Debug, "Invalid Base64 String, rc = %s, id=%d", rc, idx);
        delete rc;
        return nullptr;
    }
    size = (dst - rc);
    return rc;
}

SiteStats::SiteStats(const std::string& hostName)
    : m_hostName{std::move(hostName)}
    , m_rxBytes{0}
    , m_txBytes{0}
    , m_totalTime{0}
    , m_connections{0}
{}

const std::string& SiteStats::GetHostName() {
    return m_hostName;
}

void SiteStats::AddRxBytes(std::uint64_t rxBytes) {
    m_rxBytes += rxBytes;
}

void SiteStats::AddTxBytes(std::uint64_t txBytes) {
    m_txBytes += txBytes;
}

void SiteStats::IncrementConnections() {
    m_connections++;
}

void SiteStats::AddConnectionTime(std::uint64_t timeMs) {
    m_totalTime += timeMs;
}

std::uint64_t SiteStats::GetRxBytes() const {
    return m_rxBytes;
}

std::uint64_t SiteStats::GetTxBytes() const {
    return m_txBytes;
}

std::uint64_t SiteStats::GetTotalTime() const {
    return m_totalTime;
}

void SiteStats::Print() const {
    Log(LogSeverity::Debug, "Host: %s\nRxBytes: %llu\nTxBytes: %llu\nTotalTimeMs: %llu\nConnections: %d",
        m_hostName.c_str(), m_txBytes, m_rxBytes, m_totalTime, m_connections);
}

UserStats::UserStats(const std::string& userName) :
    m_userName{userName}
{}

SiteStats& UserStats::GetStats(const std::string& host) {
    for (auto& stat : m_stats) {
        if (stat.GetHostName() == host) {
            return stat;
        }
    }
    m_stats.emplace_back(SiteStats{host});
    return m_stats.back();
}

void SiteStats::LogToFile(int fd) const {
    char tmpBuf[1024];
    snprintf(tmpBuf, 1024, "%s,%lu,%lu,%lu,%d\n",
             m_hostName.c_str(),
             m_rxBytes,
             m_txBytes,
             m_totalTime,
             m_connections);
    auto rc = ::write(fd, tmpBuf, strlen(tmpBuf));
    if (rc !=  strlen(tmpBuf)) {
        Log(LogSeverity::Warn, "Error writing audit log %d=%d", rc, errno);
    }
}

const std::string& UserStats::GetUserName() {
    return m_userName;
}
void UserStats::LogToFile()
{
    // Don't log stats for users who haven't connected in this period
    if (m_stats.size() == 0) {
        return;
    }

    char fileName[256];
    snprintf(fileName, sizeof(fileName), "%s_%d.csv", m_userName.c_str(), s_logNumber);
    Log(LogSeverity::Debug, "Opening file %s for logging", fileName);
    auto fd = ::open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWUSR);
    if (fd < 0) {
        Log(LogSeverity::Warn, "Error opening log file for write %d(%s)", errno, strerror(errno));
        return;
    }
    constexpr auto* headerString = "host,rxBytes,txBytes,totalTime,connections\n";

    auto rc = ::write(fd, headerString, strlen(headerString));
    if (rc == -1) {
        Log(LogSeverity::Warn, "Error writing log header");
        ::close(fd);
        return;
    }

    for (auto& stat : m_stats) {
        stat.LogToFile(fd);
    }

    ::close(fd);
}

int UserStats::GetStatCount() {
    return m_stats.size();
}

GlobalStats::GlobalStats()
{
    m_lastLogTime = Timestamp();
}

GlobalStats& GlobalStats::Instance() {
    static GlobalStats* instance = new GlobalStats;
    return *instance;
}

SiteStats& GlobalStats::GetStats(const std::string& username, const std::string& host) {
    for (auto& stat : m_userStats) {
        if (stat.GetUserName() == username) {
            return stat.GetStats(host);
        }
    }
    m_userStats.emplace_back(UserStats{username});
    auto& stat = m_userStats.back();
    return stat.GetStats(host);
}

void GlobalStats::DumpUserStats(const std::string& username) {
    for (auto& stat : m_userStats) {
        if (stat.GetUserName() == username) {
            stat.LogToFile();
            return;
        }
    }
}

void GlobalStats::LogAndReset() {
    m_lastLogTime = Timestamp();

    for (auto& stat : m_userStats) {
        stat.LogToFile();
    }

    m_userStats.clear();
    s_logNumber++;
    if (s_logNumber > m_logPersistence) {
        s_logNumber = 0;
    }
}

bool GlobalStats::ReadyToLog() {
    if ((Timestamp() - m_lastLogTime) > (1000 * m_logFrequency)) {
        return true;
    }
    return false;
}
