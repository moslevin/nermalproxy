#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Log.hpp"

static constexpr auto m_intervalTime = 30; // minutes

static constexpr int MinutesToIntervals(int hours, int minutes)  {
    return ((hours * 60) + minutes) / m_intervalTime;
}

static constexpr auto m_intervals = MinutesToIntervals(24, 0);

class TimeMap {
public:
    TimeMap();

    bool InitFromString(const std::string& initString);
    bool PermittedAtTime(int hour, int minute);
    void Print();

private:
    bool m_accessIntervals[m_intervals];
};

class WeeklyAccess {
public:
    bool InitDayFromString(int day, const std::string& initString);
    bool PermittedAtTime(int day, int hour, int minute);

private:
    static constexpr auto m_days = 7;
    TimeMap m_accessMap[m_days];
};
