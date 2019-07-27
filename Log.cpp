#include "Log.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "Timestamp.hpp"

namespace {
    auto m_verbosity = LogSeverity::Debug;
    auto m_toFile = false;
} // anonymous namespace

void Log(LogSeverity severity, const char* format, ...) {
    if (static_cast<int>(m_verbosity) < static_cast<int>(severity)) {
        return;
    }
    static FILE* logFile = nullptr;

    char buf[1024];
    char* tmpBuf = buf;

    auto ts = Timestamp();
    tmpBuf += sprintf(tmpBuf, "[%04lu.%03lu]", ts / 1000, ts % 1000);
    switch (severity) {
    case LogSeverity::Error: {
        tmpBuf += sprintf(tmpBuf, "\tERR\t");
    } break;
    case LogSeverity::Warn: {
        tmpBuf += sprintf(tmpBuf, "\tWRN\t");
    } break;
    case LogSeverity::Info: {
        tmpBuf += sprintf(tmpBuf, "\tNFO\t");
    } break;
    case LogSeverity::Debug: {
        tmpBuf += sprintf(tmpBuf, "\tDBG\t");
    } break;
    }

    va_list args;

    va_start(args, format);
    tmpBuf += vsprintf(tmpBuf, format, args);
    va_end(args);
    *tmpBuf = '\0';
    printf("%s\n", buf);
    if (m_toFile) {
        if (logFile == nullptr) {
            logFile = fopen("logs.txt", "w");
        }
        fprintf(logFile, "%s\n", buf);
    }
}

void LogSetVerbosity(LogSeverity verbosity) {
    m_verbosity = verbosity;
}

void LogStoreToDisk(bool enable) {
    m_toFile = true;
}
