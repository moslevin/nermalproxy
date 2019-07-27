#pragma once

#include "IGenericSocket.hpp"

#include <cstddef>
#include <cstdint>

class ClientSocket : public IGenericSocket
{
public:
    ClientSocket(const int socketFd, const int proxyFd, const int sessionId, const bool clientToProxy);
    virtual ~ClientSocket();

    int GetFd() const override;
    SocketType Identity() const override;

    int GetSessionId();
    std::size_t Read(void* readBuf_, std::size_t bytesToRead_, bool* isError_);
    std::size_t Write(const void* writeBuf_, std::size_t bytesToWrite_, bool* isError_);
    int GetProxyFd();    

    bool IsClientToProxy();

private:
    void OnError();

    bool m_isActive;
    bool m_isClientToProxy;
    int m_socketFd;
    int m_peerFd;
    int m_sessionId;
};
