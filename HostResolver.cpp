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

