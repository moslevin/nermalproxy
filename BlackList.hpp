#pragma once

#include <list>
#include <set>
#include <memory>
#include <string>

#include "Log.hpp"

class Blacklist {
public:
    Blacklist(const std::string& name);

    void Clear();

    bool LoadFromFile(const std::string& path);
    const std::string& GetName();
    bool IsHostAllowed(const std::string& host);

private:
    std::string m_name;
    std::set<std::string> m_blacklist;
};

class BlacklistList {
public:
    static BlacklistList& Instance();

    void AddList(std::unique_ptr<Blacklist> list);
    bool IsAllowedInList(const std::string& name, const std::string& host);
    bool HasList(const std::string& name);

private:
    std::list<std::unique_ptr<Blacklist>> m_blacklists;
};
