#include "UserAuth.hpp"

#include <string>
#include <string.h>
#include <list>
#include <vector>

#include "Log.hpp"

User::User(const std::string& name, const std::string& password)
    : m_name{std::move(name)}
    , m_password{std::move(password)}
    , m_ipList{}
{}

void User::AddIp(const std::string& ipAddress) {
    m_ipList.emplace_back(ipAddress);
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

bool AuthManager::SetEnabled(bool enable) {
    m_enabled = enable;
}

bool AuthManager::IsEnabled() {
    return m_enabled;
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
    for (auto& user: m_users) {
        if (user.HasIpList()) {
            auto ipList = user.GetIpList();
            for (auto& ip: ipList) {
                if (ip == ipAddress) {
                    username = user.GetName();
                    return true;
                }
            }
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

const std::string& UserStats::GetUserName() {
    return m_userName;
}

void UserStats::DumpUserStats() {
    for (auto& stat : m_stats) {
        stat.Print();
    }
}

int UserStats::GetStatCount() {
    return m_stats.size();
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
            stat.DumpUserStats();
            return;
        }
    }
}

void GlobalStats::LogAndReset() {
    for (auto& stat : m_userStats) {
        stat.DumpUserStats();
    }
    m_userStats.clear();
}
