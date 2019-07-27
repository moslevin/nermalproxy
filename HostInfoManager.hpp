#pragma once

#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

typedef struct {
    std::string url;
    std::string address;
    std::uint8_t ipVersion;
    std::uint64_t Timestamp;
} HostInfo;

class HostInfoCache {
public:
    std::vector<HostInfo> GetCachedResults(const std::string& url);
    void AddResult(const HostInfo& hostInfo);
    void ClearCacheForHost(const std::string& url);

private:
    using LockGuard = std::unique_lock<std::mutex>;

    static constexpr auto maxCacheSize = 2048;
    static constexpr auto maxAge = 1800 * 1000; //ms

    std::list<HostInfo> m_hostInfo;

    std::unique_ptr<HostInfoCache> m_hostCache;
    std::mutex m_cacheLock;
};
