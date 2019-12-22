
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

#include <list>
#include <set>
#include <memory>
#include <string>

#include "Log.hpp"

/**
 * @brief The Blacklist class implements a set of domains
 * that are blocked.
 */
class Blacklist {
public:
    Blacklist(const std::string& name);

    // Clear the list
    void Clear();

    // Load the contents of the blacklist from a file containing hosts, 1 per line
    bool LoadFromFile(const std::string& path);

    // Get the name of the blacklist
    const std::string& GetName();

    // Check to see whether or not a domain is allowed or blocked in this list
    bool IsHostAllowed(const std::string& host);

private:
    std::string m_name;
    std::set<std::string> m_blacklist;
};

/**
 * @brief The BlacklistList class is used to manage a collection of blacklists
 */
class BlacklistList {
public:
    static BlacklistList& Instance();

    // Add a blacklist to the object
    void AddList(std::unique_ptr<Blacklist> list);

    // Check to see if a host is allowed in a specific list
    bool IsAllowedInList(const std::string& name, const std::string& host);

    // Check to see if a blacklist exists in the object
    bool HasList(const std::string& name);

private:
    std::list<std::unique_ptr<Blacklist>> m_blacklists;
};
