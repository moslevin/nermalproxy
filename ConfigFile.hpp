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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <list>
#include <string>

#include "Log.hpp"

/**
 * @brief The MultiValueAttribute class implements a data structure
 * containing one key, and 0 or more values.
 */
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

/**
 * @brief The ConfigSection class implements a collection of
 * configuration data attributes.
 */
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

/**
 * @brief The ConfigData class represents the program configuration data,
 * as a collection of sections, each containing unique attributes.
 */
class ConfigData {
public:

    ConfigSection& AddSection(const std::string& name);
    ConfigSection& FindSection(const std::string& name);

private:
    std::list<ConfigSection> m_sections;
};

/**
 * @brief The ConfigFile class loads a file with a given format into a
 * ConfigData object.
 */
class ConfigFile {
public:
    ConfigFile(const std::string& path);

    void LoadData();
    ConfigSection& GetSection(const std::string& name);

private:
    std::string m_path;
    ConfigData m_data;
};
