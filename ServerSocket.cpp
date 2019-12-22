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

#include "ServerSocket.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
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

    rc = ::listen(sfd, 16);
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

    // Set a timeout on the socket when reading/writing to prevent getting stuck waiting for
    // data on an idle socket.
    struct timeval timeout = {};
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    ::setsockopt(clifd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    ::setsockopt(clifd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    return clifd;
}

void ServerSocket::OnError() {
    if (m_isActive) {
        m_isActive = false;
        m_socketFd = -1;
    }
}
