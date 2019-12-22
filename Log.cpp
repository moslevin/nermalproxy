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
