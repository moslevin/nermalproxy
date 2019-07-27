#pragma once

#include "IHostResolver.hpp"

#include <memory>
#include <vector>
#include <string>

class HostResolver : public IHostResolver {
public:
    static HostResolver& Instance() {
        static HostResolver* instance = new HostResolver{std::make_unique<HostInfoCache>()};
        return *instance;
    }

    HostResolver(std::unique_ptr<HostInfoCache> cache);
    std::vector<HostInfo> GetAddressesForHost(const std::string &host, const uint16_t port, bool& cached) override;
    void ClearCacheForHost(const std::string& host) override;

public:
    std::unique_ptr<HostInfoCache> m_hostCache;
};
