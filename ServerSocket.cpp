#include "ServerSocket.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <linux/sockios.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include "Log.hpp"

ServerSocket::ServerSocket()
    : m_isActive{false}
    , m_socketFd{-1}
{}

ServerSocket::~ServerSocket() {
    if (m_isActive) {
        ::close(m_socketFd);
    }
}

int ServerSocket::GetFd() const  {
    if (m_isActive) {
        return m_socketFd;
    }
    return -1;
}

SocketType ServerSocket::Identity() const {
    return SocketType::Server;
}

bool ServerSocket::Initialize(std::uint16_t port) {
    auto sfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
        Log(LogSeverity::Debug, "error on socket");
        return -1;
    }

    int enable = 1;
    auto rc = ::setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (rc < 0) {
        Log(LogSeverity::Debug, "error on setsockopt");
        return false;
    }

    auto sockaddr = sockaddr_in{};
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    rc = ::bind(sfd, reinterpret_cast<struct sockaddr*>(&sockaddr), sizeof(sockaddr));
    if (rc < 0) {
        Log(LogSeverity::Debug, "error on bind");
        return false;
    }

    rc = ::listen(sfd, 10);
    if (rc < 0) {
        Log(LogSeverity::Debug, "error on listen");
        return false;
    }

    m_isActive = true;
    m_socketFd = sfd;
    return true;
}

int ServerSocket::Accept(std::string& clientIp) {
    if (!m_isActive) {
        return -1;
    }

    auto cliaddr = sockaddr_storage{};
    auto clilen = socklen_t{sizeof(cliaddr)};
    auto clifd = ::accept(m_socketFd, reinterpret_cast<struct sockaddr*>(&cliaddr), &clilen);
    if (clifd < 0) {
        Log(LogSeverity::Debug, "error on accept");
        return -1;
    }

    char clientIpBuf[INET6_ADDRSTRLEN];
    if (cliaddr.ss_family == AF_INET) {
        auto *addrv4 = reinterpret_cast<struct sockaddr_in*>(&cliaddr);
        inet_ntop(AF_INET, &addrv4->sin_addr, clientIpBuf, sizeof clientIpBuf);
    } else {
        auto *addrv6 = reinterpret_cast<struct sockaddr_in6*>(&cliaddr);
        inet_ntop(AF_INET6, &addrv6->sin6_addr, clientIpBuf, sizeof clientIpBuf);
    }

    Log(LogSeverity::Debug, "Peer IP address: %s\n", clientIpBuf);
    clientIp = std::string(clientIpBuf);

    return clifd;
}

void ServerSocket::OnError() {
    if (m_isActive) {
        m_isActive = false;
        m_socketFd = -1;
    }
}
