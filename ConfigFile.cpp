#include "ConfigFile.hpp"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <list>
#include <string>

#include "Log.hpp"

MultiValueAttribute::MultiValueAttribute(const std::string& name) :
    m_name{name}
{}

void MultiValueAttribute::AddValue(const std::string& value) {
    m_values.emplace_back(value);
}

const std::string& MultiValueAttribute::GetName() {
    return m_name;
}

bool MultiValueAttribute::GetValue(const int index, std::string& value) {
    if (index > m_values.size()) {
        return false;
    }

    auto i = 0;
    for (auto it = m_values.begin(); it != m_values.end(); it++, i++) {
        if (i == index) {
            value = *it;
            return true;
        }
    }
    return false;
}

std::string MultiValueAttribute::GetValue() {
    if (!m_values.size()) {
        return "";
    }

    return m_values.front();
}


int MultiValueAttribute::GetCount()  {
    return m_values.size();
}


ConfigSection::ConfigSection(const std::string& sectionName) :
    m_name{sectionName}
{}

const std::string& ConfigSection::GetName() {
    return m_name;
}

MultiValueAttribute& ConfigSection::GetAttribute(const std::string& attribute) {
    for (auto& attr : m_attributes) {
        if (attr.GetName() == attribute) {
            return attr;
        }
    }
    m_attributes.emplace_back(attribute);
    return m_attributes.back();
}

MultiValueAttribute* ConfigSection::GetAttribute(int idx) {
    int i = 0;
    for (auto it = m_attributes.begin(); it != m_attributes.end(); it++, i++) {
        if (i == idx) {
            return &(*it);
        }
    }
    return nullptr;
}

ConfigSection& ConfigData::AddSection(const std::string& name) {
    for (auto& section : m_sections) {
        if (section.GetName() == name) {
            return section;
        }
    }
    m_sections.emplace_back(ConfigSection{name});
    return m_sections.back();
}

ConfigSection& ConfigData::FindSection(const std::string& name) {
    for (auto& section : m_sections) {
        if (section.GetName() == name) {
            return section;
        }
    }
    return AddSection(name);
}

ConfigFile::ConfigFile(const std::string& path) :
    m_path{path}
{}

void ConfigFile::LoadData() {
    auto* file = fopen(m_path.c_str(), "r");
    if (file == nullptr) {
        return;
    }
    char* lineBuf = {};
    char sectionName[1024] = {};
    int line = 1;
    size_t size = 1024;
    auto done = false;
    while (!done) {
        auto rc = getline(&lineBuf, &size, file);
        if (rc == -1) {
            done = true;
        } else {
            // Prune out comments (lines starting with #)
            auto* commentPtr = strchr(lineBuf, '#');
            if (commentPtr) {
                *commentPtr = '\0';
            }

            // Check for section specifiers...
            auto* sectionPtr = strchr(lineBuf, '[');
            auto* sectionEndPtr = strrchr(lineBuf, ']');
            if (sectionPtr && sectionEndPtr) {
                *sectionPtr = '\0';
                *sectionEndPtr = '\0';
                auto nread = sscanf(sectionPtr + 1, "%s", sectionName);
                if (!nread) {
                    done = true;
                } else {
                    m_data.AddSection({sectionName});
                }
            } else {
                // Check for key/value specifiers... (in the format "key:value1, value2, ..., valueN")
                char keyName[1024];
                char value[1024];
                auto valueDone = false;
                auto* valuePtr = strchr(lineBuf, ':');

                if (!valuePtr) {
                    if (1 == sscanf(lineBuf, "%s", value)) {
                        done = true;
                    }
                } else {
                    *valuePtr = '\0';
                    sscanf(lineBuf, "%s", keyName);

                    auto* valuetmp = valuePtr + 1;
                    while (!valueDone) {
                        auto* nextValue = strchr(valuetmp, ',');
                        if (!nextValue) {
                            valueDone = true;
                            auto rc = sscanf(valuetmp, "%s", value);
                            m_data.FindSection({sectionName}).GetAttribute({keyName}).AddValue({value});
                        } else {
                            *nextValue = '\0';
                            auto rc = sscanf(valuetmp, "%s", value);
                            m_data.FindSection({sectionName}).GetAttribute({keyName}).AddValue({value});
                            valuetmp = nextValue + 1;
                        }
                    }
                }
            }
            free(lineBuf);
            lineBuf = nullptr;
        }
    }
    fclose(file);
}

ConfigSection& ConfigFile::GetSection(const std::string& name) {
    return m_data.FindSection(name);
}
