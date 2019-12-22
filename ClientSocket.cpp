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

#include "ClientSocket.hpp"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#include "Log.hpp"

ClientSocket::ClientSocket(const int socketFd, const int proxyFd, const int sessionId, const bool isClientToProxy)
    : m_isActive{true}
    , m_isClientToProxy{isClientToProxy}
    , m_socketFd{socketFd}
    , m_peerFd{proxyFd}
    , m_sessionId{sessionId}
{}

ClientSocket::~ClientSocket() {
    ::close(m_socketFd);
    m_socketFd = -1;
}
int ClientSocket::GetFd() const {
    return m_socketFd;
}

SocketType ClientSocket::Identity() const {
    return SocketType::Client;
}

int ClientSocket::GetSessionId() {
    return m_sessionId;
}

std::size_t ClientSocket::Read(void* readBuf_, std::size_t bytesToRead_, bool* isError_) {
    if (!m_isActive) {
        if (isError_) {
            *isError_ = true;
        }
        return 0;
    }

    auto done = false;
    auto numRead = size_t{};
    while (!done) {
        numRead = ::read(m_socketFd, readBuf_, bytesToRead_);
        if (numRead <= 0) {
            if ((numRead == -1) && ((errno == EAGAIN) || (errno == EINTR))) {
                // Recoverable errors
                Log(LogSeverity::Debug, "::read - numRead=%d, errno=%d (%s)", numRead, errno, ::strerror(errno));
                continue;
            }

            if (isError_) {
                *isError_ = true;
            }
            Log(LogSeverity::Debug, "::read error numRead=%d, errno=%d (%s)", numRead, errno, ::strerror(errno));
            OnError();
        }
        done = true;
    }
    if (isError_) {
        *isError_ = false;
    }
    Log(LogSeverity::Verbose, "Session %d : fd=%d read %d bytes", m_sessionId, m_socketFd, numRead);
    return numRead;
}

std::size_t ClientSocket::Write(const void* writeBuf_, std::size_t bytesToWrite_, bool* isError_) {
    if (!m_isActive) {
        if (isError_) {
            *isError_ = true;
        }
        return 0;
    }

    const auto* writePtr = static_cast<const std::uint8_t*>(writeBuf_);
    auto totalWritten = std::size_t{};

    while (bytesToWrite_) {

        auto numWritten = ::write(m_peerFd, writePtr, bytesToWrite_);
        if (numWritten <= 0) {
            // Recoverable errors
            if ((numWritten == -1) && ((errno == EAGAIN) || (errno == EINTR))) {
                continue;
            }

            if (isError_) {
                *isError_ = true;
            }
            Log(LogSeverity::Debug, "::write error - numRead=%d, errno=%d", numWritten, errno);
            OnError();
            return 0;
        }
        bytesToWrite_ -= numWritten;
        writePtr += numWritten;
        totalWritten += numWritten;
    }

    if (isError_) {
        *isError_ = false;
    }
    Log(LogSeverity::Verbose, "Session %d : fd=%d wrote %d bytes", m_sessionId, m_socketFd, totalWritten);
    return totalWritten;
}

int ClientSocket::GetProxyFd() {
    return m_peerFd;
}

bool ClientSocket::IsClientToProxy() {
    return m_isClientToProxy;
}

void ClientSocket::OnError() {
    if (m_isActive) {
        m_isActive = false;
    }
}
