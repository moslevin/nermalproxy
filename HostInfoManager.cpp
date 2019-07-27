#include "HostInfoManager.hpp"

#include "Log.hpp"
#include "Timestamp.hpp"

std::vector<HostInfo> HostInfoCache::GetCachedResults(const std::string& url) {
    auto lg = LockGuard{m_cacheLock};
    auto results = std::vector<HostInfo>{};
    for (auto it = m_hostInfo.begin(); it != m_hostInfo.end(); it++) {
        if ((Timestamp() - it->Timestamp) > maxAge) {
            Log(LogSeverity::Debug, "pruning old entry: %s->%s", it->url.c_str(), it->address.c_str());
            m_hostInfo.erase(it++);
        } else if (it->url == url) {
            Log(LogSeverity::Debug, "cache hit for : %s->%s", it->url.c_str(), it->address.c_str());
            results.push_back(*it);
        }
    }
    return results;
}

void HostInfoCache::ClearCacheForHost(const std::string& url) {
    auto lg = LockGuard{m_cacheLock};
    for (auto it = m_hostInfo.begin(); it != m_hostInfo.end(); it++) {
        if (it->url == url) {
            m_hostInfo.erase(it++);
        }
    }
}

void HostInfoCache::AddResult(const HostInfo& hostInfo) {
    auto lg = LockGuard{m_cacheLock};
    m_hostInfo.push_back(hostInfo);
    if (m_hostInfo.size() > maxCacheSize) {
        m_hostInfo.pop_front();
    }
}
