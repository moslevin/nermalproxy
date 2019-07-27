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
