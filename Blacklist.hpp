#pragma once

#include <list>
#include <set>
#include <memory>
#include <string>

#include "Log.hpp"

class Blacklist {
public:
    Blacklist(const std::string& name) :
        m_name{name}
    {}

    void Clear() {
        m_blacklist.clear();
    }

    bool LoadFromFile(const std::string& path) {
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

    const std::string& GetName() {
        return m_name;
    }

    bool IsHostAllowed(const std::string& host) {
        if (m_blacklist.find(host) == m_blacklist.end()) {
            return true;
        }
        return false;
    }

private:
    std::string m_name;
    std::set<std::string> m_blacklist;
};

class BlacklistList {
public:
    static BlacklistList& Instance() {
        static BlacklistList* instance = new BlacklistList();
        return *instance;
    }

    void AddList(std::unique_ptr<Blacklist> list) {
        m_blacklists.emplace_back(std::move(list));
    }

    bool IsAllowedInList(const std::string& name, const std::string& host) {
        for (auto& list : m_blacklists) {
            if (name == list->GetName()) {
                return list->IsHostAllowed(host);
            }
        }
        return false; // No list for name...
    }

    bool HasList(const std::string& name) {
        for (auto& list : m_blacklists) {
            if (name == list->GetName()) {
                return true;
            }
        }
        return false; // No list for name...
    }

private:
    std::list<std::unique_ptr<Blacklist>> m_blacklists;
};
