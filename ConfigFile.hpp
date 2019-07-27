#pragma once

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <list>
#include <string>

#include "Log.hpp"

class MultiValueAttribute {
public:
    MultiValueAttribute(const std::string& name);

    void AddValue(const std::string& value);
    const std::string& GetName();
    bool GetValue(const int index, std::string& value);
    std::string GetValue();
    int GetCount();

public:
    std::string m_name;
    std::list<std::string> m_values;
};

class ConfigSection {
public:
    ConfigSection(const std::string& sectionName);

    const std::string& GetName();
    MultiValueAttribute& GetAttribute(const std::string& attribute);
    MultiValueAttribute* GetAttribute(int idx);

private:
    std::string m_name;
    std::list<MultiValueAttribute> m_attributes;
};

class ConfigData {
public:

    ConfigSection& AddSection(const std::string& name);
    ConfigSection& FindSection(const std::string& name);

private:
    std::list<ConfigSection> m_sections;
};

class ConfigFile {
public:
    ConfigFile(const std::string& path);

    void LoadData();
    ConfigSection& GetSection(const std::string& name);

private:
    std::string m_path;
    ConfigData m_data;
};
