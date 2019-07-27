#pragma once

#include "IGenericSocket.hpp"
#include <string>

class ServerSocket : public IGenericSocket
{
public:
    ServerSocket();
    ~ServerSocket();

    int GetFd() const override;
    SocketType Identity() const override;

    bool Initialize(std::uint16_t port);
    int Accept(std::string& clientIp);

private:
    void OnError();

    int m_socketFd;
    bool m_isActive;
};
