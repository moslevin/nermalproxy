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

#include <cstdint>
#include <string>
#include <list>
#include <mutex>

#include "UserAuth.hpp"
#include "Timestamp.hpp"

/**
 * @brief The Session class provides information about a unique instance of
 * a proxy connection.
 */
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

/**
 * @brief The SessionManager class manages the lifecycle of active session
 * objects in the system.
 */
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
