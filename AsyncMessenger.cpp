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

#include "AsyncMessenger.hpp"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "Log.hpp"

AsyncMessenger::~AsyncMessenger() {
    close(m_pipeFds[readPipe]);
    close(m_pipeFds[writePipe]);
}

int AsyncMessenger::GetReadFd() {
    return m_pipeFds[readPipe];
}

bool AsyncMessenger::WriteMessage(const AsyncMessage_t& message) {
    auto lg = LockGuard{m_writeLock};

    auto numWritten = ::write(m_pipeFds[writePipe], &message, sizeof(message));
    if (numWritten == sizeof(message)) {
        return true;
    }
    return false;
}

void AsyncMessenger::ReadMessage(AsyncMessage_t& message) {
    auto rc = ::read(m_pipeFds[readPipe], &message, sizeof(message));
    if (rc == -1) {
        Log(LogSeverity::Error, "%s: Error writing message, errno=%d", __func__, errno);
    }
}

AsyncMessenger::AsyncMessenger() {
    auto rc = ::pipe(m_pipeFds);
    if (rc != 0) {
        Log(LogSeverity::Error, "%s: Error creating pipe", __func__);
        exit(-1);
    }
}
