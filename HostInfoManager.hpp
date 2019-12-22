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

#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Simple struct of objects representing a single host->IP adress mapping
typedef struct {
    std::string url;
    std::string address;
    std::uint8_t ipVersion;
    std::uint64_t Timestamp;
} HostInfo;

/**
 * @brief The HostInfoCache class implements a simple (unoptimized) LRU-style
 * cache of host info objects (DNS lookups) used to speed-up the connection
 * process.  Old entries are purged on connection failure, or on expiry.
 */
class HostInfoCache {
public:
    // Get the cached host info results for a given host
    std::vector<HostInfo> GetCachedResults(const std::string& url);    

    // Add a host lookup entry to the cache
    void AddResult(const HostInfo& hostInfo);

    // Force-remove a host lookup entry by host name - used to purge cache
    // for a host that failed to connect using the cached entry.
    void ClearCacheForHost(const std::string& url);

private:
    using LockGuard = std::unique_lock<std::mutex>;

    // Constants defining the maximum size of cache and age of entries.
    static constexpr auto maxCacheSize = 2048;
    static constexpr auto maxAge = 1800 * 1000; //ms

    // ToDo: Replace with an actual LRU cache.
    std::list<HostInfo> m_hostInfo;
    std::mutex m_cacheLock;
};
