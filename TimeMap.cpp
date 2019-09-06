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
