#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "HostInfoManager.hpp"

class IHostResolver {
public:
    ~IHostResolver() {}
    virtual std::vector<HostInfo> GetAddressesForHost(const std::string &host, const std::uint16_t port, bool& cached) = 0;
    virtual void ClearCacheForHost(const std::string& host) = 0;
};
