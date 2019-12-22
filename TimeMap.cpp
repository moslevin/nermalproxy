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

#include "TimeMap.hpp"

TimeMap::TimeMap() {
    for (auto i = 0; i < m_intervals; i++) {
        m_accessIntervals[i] = true;
    }
}

bool TimeMap::InitFromString(const std::string& initString) {
    if (initString.size() != m_intervals) {
        Log(LogSeverity::Warn, "Invalid number of entries!");
        return false;
    }
    for (auto i = 0; i < (initString.size()) && (i < m_intervals); i++) {
        if ((initString.at(i) == 'x') || (initString.at(i) == 'X')) {
            m_accessIntervals[i] = false;
        } else {
            m_accessIntervals[i] = true;
        }
    }
    Log(LogSeverity::Debug, "Init OK");
    return true;
}

bool TimeMap::PermittedAtTime(int hour, int minute) {
    auto interval = MinutesToIntervals(hour, minute);
    if (interval >= m_intervals) {
        Log(LogSeverity::Debug, "Internet not permitted at interval %d %d:%02d", interval, hour, minute);
        return false;
    }
    return m_accessIntervals[interval];
}

void TimeMap::Print() {
    char timeView[m_intervals + 1] = {};
    for (auto i = 0; i < m_intervals; i++) {
        if (m_accessIntervals[i] == true) {
            timeView[i] = '-';
        } else {
            timeView[i] = 'X';
        }
    }
    Log(LogSeverity::Debug, "TimeView: %s", timeView);
}

bool WeeklyAccess::InitDayFromString(int day, const std::string& initString) {
    if (day > m_days) {
        Log(LogSeverity::Warn, "Invalid day");
        return false;
    }
    return m_accessMap[day].InitFromString(initString);
}

bool WeeklyAccess::PermittedAtTime(int day, int hour, int minute) {
    if (day > m_days) {
        Log(LogSeverity::Warn, "%s: invalid day index %d", __func__, day);
        return false;
    }
    return m_accessMap[day].PermittedAtTime(hour, minute);
}
