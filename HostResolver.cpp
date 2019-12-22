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

#include "HostResolver.hpp"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "Log.hpp"
#include "Timestamp.hpp"

HostResolver::HostResolver(std::unique_ptr<HostInfoCache> cache)
    : m_hostCache{std::move(cache)}
{}

std::vector<HostInfo> HostResolver::GetAddressesForHost(const std::string &host, const uint16_t port, bool& cached) {

    auto resultVector = std::vector<HostInfo>{};
    resultVector = m_hostCache->GetCachedResults(host);

    if (resultVector.size() > 0) {
        cached = true;
        return resultVector;
    }

    auto hints = addrinfo{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    auto* results = (addrinfo*){};
    auto portString = std::to_string(port);
    auto rc = getaddrinfo(host.c_str(), portString.c_str(), &hints, &results);

    if (rc != 0) {
        Log(LogSeverity::Debug, "could not resolve address");
        return {};
    }

    for (auto result = results; result != nullptr; result = result->ai_next) {
        char ipString[INET6_ADDRSTRLEN] = {0};

        auto hostInfo = HostInfo{};
        if (result->ai_family == AF_INET) {
            // optimization -- only take the first IPv4 result.
            auto* addr = (struct sockaddr_in*)(result->ai_addr);
            hostInfo.ipVersion = 4;
            inet_ntop(AF_INET, &(addr->sin_addr), ipString, sizeof(ipString));

            hostInfo.address = std::string{ipString};
            hostInfo.url = host;
            hostInfo.Timestamp = Timestamp();

            resultVector.push_back(hostInfo);
            m_hostCache->AddResult(hostInfo);

            break;
        }
#if 0 // Do we want to deal with IPv6 results?
        else {
            auto* addr = (struct sockaddr_in6*)(result->ai_addr);
            hostInfo.ipVersion = 6;
            inet_ntop(AF_INET6, &(addr->sin6_addr), ipString, sizeof(ipString));
        }
#endif
    }

    freeaddrinfo(results);
    return resultVector;
}

void HostResolver::ClearCacheForHost(const std::string& host) {
    m_hostCache->ClearCacheForHost(host);
}

