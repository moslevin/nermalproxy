#include "BlackList.hpp"

#include <list>
#include <set>
#include <memory>
#include <string>
#include <string.h>

Blacklist::Blacklist(const std::string& name) :
    m_name{name}
{}

void Blacklist::Clear() {
    m_blacklist.clear();
}

bool Blacklist::LoadFromFile(const std::string& path) {
    // Read line by line...
    auto* file = fopen(path.c_str(), "r");
    if (!file) {
        Log(LogSeverity::Debug, "Can't open blacklist (%s) file %s", m_name.c_str(), path.c_str());
        return false;
    }

    char* linebuf = nullptr;
    size_t lineSize = 0;
    int line = 0;
    Log(LogSeverity::Debug, "Opened blacklist (%s) file: %s, reading", m_name.c_str(), path.c_str());
    while (-1 != getline(&linebuf, &lineSize, file)) {
        auto* endchar = strchr(linebuf, '\r');
        if (endchar != nullptr) {
            *endchar = '\0';
        }
        endchar = strchr(linebuf, '\n');
        if (endchar != nullptr) {
            *endchar = '\0';
        }
        m_blacklist.insert(std::string{linebuf});
        free(linebuf);
        linebuf = nullptr;
        line++;
        if ((line % 100000) == 0) {
            Log(LogSeverity::Debug, "Processed %d entries..., ", line);
        }
    }
    Log(LogSeverity::Info, "Successfully processed %d rules for blacklist (%s) from %s", line, m_name.c_str(), path.c_str());
    return true;
}

const std::string& Blacklist::GetName() {
    return m_name;
}

bool Blacklist::IsHostAllowed(const std::string& host) {
    if (m_blacklist.find(host) == m_blacklist.end()) {
        return true;
    }
    return false;
}

BlacklistList& BlacklistList::Instance() {
    static BlacklistList* instance = new BlacklistList();
    return *instance;
}

void BlacklistList::AddList(std::unique_ptr<Blacklist> list) {
    m_blacklists.emplace_back(std::move(list));
}

bool BlacklistList::IsAllowedInList(const std::string& name, const std::string& host) {
    for (auto& list : m_blacklists) {
        if (name == list->GetName()) {
            return list->IsHostAllowed(host);
        }
    }
    return false; // No list for name...
}

bool BlacklistList::HasList(const std::string& name) {
    for (auto& list : m_blacklists) {
        if (name == list->GetName()) {
            return true;
        }
    }
    return false; // No list for name...
}
