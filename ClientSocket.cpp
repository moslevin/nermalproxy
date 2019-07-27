#include "ClientSocket.hpp"

#include <errno.h>
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
