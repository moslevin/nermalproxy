#pragma once

#include <cstdint>
#include <stdarg.h>

enum class LogSeverity : std::uint8_t {
    Error,
    Warn,
    Info,
    Debug,
    Verbose
};

void Log(LogSeverity severity, const char* format, ...);

void LogSetVerbosity(LogSeverity verbosity);

void LogStoreToDisk(bool enable);

