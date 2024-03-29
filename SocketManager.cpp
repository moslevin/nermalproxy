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

#include "SocketManager.hpp"

#include <fcntl.h>
#include <linux/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "AsyncMessenger.hpp"
#include "ClientSocket.hpp"
#include "CommandSocket.hpp"
#include "ServerSocket.hpp"
#include "HostInfoManager.hpp"
#include "Session.hpp"
#include "UserAuth.hpp"
#include "Log.hpp"

SocketManager::SocketManager(std::unique_ptr<ProxyConnector> connector)
    : m_isActive{false}
    , m_epollFd{-1}
    , m_connector{std::move(connector)}
{}

bool SocketManager::Initialize() {
    if (m_isActive) {
        return false;
    }

    auto epollFd = epoll_create1(0);
    if (-1 == epollFd) {
        return false;
    }

    m_epollFd = epollFd;
    m_isActive = true;
    return true;
}

bool SocketManager::AddSocket(std::unique_ptr<IGenericSocket> genericSocket) {
    auto event = epoll_event{};

    event.events = EPOLLIN;
    event.data.fd = genericSocket->GetFd();

    if (-1 == epoll_ctl(m_epollFd, EPOLL_CTL_ADD, event.data.fd, &event)) {
        return false;
    }

    m_sockets.emplace_back(std::move(genericSocket));
    return true;
}

bool SocketManager::RemoveSocketFd(int fd) {
    if (-1 == epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr)) {
        return false;
    }
    return true;
}

void SocketManager::PruneExcessConnections() {
    while (m_sockets.size() > 200) {
        Log(LogSeverity::Debug, "Too many active connections -- cleaning up oldest");

        for (auto& socket : m_sockets) {
            if (socket->Identity() == SocketType::Client) {
                auto* client = static_cast<ClientSocket*>(socket.get());
                auto sessionId = client->GetSessionId();

                RemoveSocketFd(socket->GetFd());
                m_sockets.remove(socket);
                auto proxyFd = client->GetProxyFd();
                for (auto& peer : m_sockets) {
                    if (peer->GetFd() == proxyFd) {
                        RemoveSocketFd(peer->GetFd());
                        m_sockets.remove(peer);
                        Log(LogSeverity::Debug, "Session: %d Destroyed sockets .", client->GetSessionId());
                        break;
                    }
                }

                SessionManager::Instance().EndSession(sessionId);
                break;
            }
        }
    }
}

bool SocketManager::HandleServerSocketRead(IGenericSocket* socket)
{
    auto* server = static_cast<ServerSocket*>(socket);
    auto clientIp = std::string{};
    auto clientFd = server->Accept(clientIp);

    if (clientFd < 0) {
        Log(LogSeverity::Debug, "Error accepting client");
        return false;
    }

    // Enable TCP keepalives and idle timeout detection on client-to-proxy communications

    auto enable = 1;
    auto rc         = setsockopt(clientFd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
    if (rc != 0) {
        printf("Error enabling socket keepalives on client\n");
    }

    // Set the timing parameters for dead "Idle" socket checks.

    // Check for dead idle connections on 10s of inactivity
    auto idleTime = 10;
    rc           = setsockopt(clientFd, SOL_TCP, TCP_KEEPIDLE, &idleTime, sizeof(idleTime));
    if (rc != 0) {
        printf("Error setting initial idle-time value\n");
    }

    // Set a maximum number of idle-socket heartbeat attemtps before assuming an idle socket it dead
    auto keepCount = 5;
    rc            = setsockopt(clientFd, SOL_TCP, TCP_KEEPCNT, &keepCount, sizeof(keepCount));
    if (rc != 0) {
        printf("Error setting idle retry count\n");
    }

    // On performing the socket-idle check, send heartbeat attempts on a specified interval
    auto keepInterval = 5;
    rc               = setsockopt(clientFd, SOL_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval));
    if (rc != 0) {
        printf("Error setting idle retry interval\n");
    }

    // Check to see if clientIp has an active user session...
    auto* session = SessionManager::Instance().CreateSession(clientFd);
    session->SetClientAddress(clientIp);
    Log(LogSeverity::Debug, "New Client fd=%d -- async proxy detection", clientFd);
    m_connector->BeginProxyDetect(session->GetSessionId());
    return true;
}

bool SocketManager::HandleClientSocketRead(IGenericSocket* socket)
{
    auto* client = static_cast<ClientSocket*>(socket);

    static std::uint8_t buf[32768];
    auto error = false;
    auto numRead = client->Read(buf, sizeof(buf), &error);
    auto sessionId = client->GetSessionId();
    if (error) {
        auto proxyFd = client->GetProxyFd();
        auto clientFd = client->GetFd();

        for (auto& peer : m_sockets) {
            if (peer->GetFd() == clientFd) {
                RemoveSocketFd(peer->GetFd());
                m_sockets.remove(peer);
                Log(LogSeverity::Debug, "Session: %d Destroyed client socket .", sessionId);
                break;
            }
        }

        for (auto& peer : m_sockets) {
            if (peer->GetFd() == proxyFd) {
                RemoveSocketFd(peer->GetFd());
                m_sockets.remove(peer);
                Log(LogSeverity::Debug, "Session: %d Destroyed proxy socket .", sessionId);
                break;
            }
        }
        SessionManager::Instance().EndSession(sessionId);
    } else {
        auto numWritten = client->Write(buf, numRead, &error);

        auto* session = SessionManager::Instance().GetSession(sessionId);
        if (client->IsClientToProxy()) {
            session->AddRxBytes(numWritten);
        } else {
            session->AddTxBytes(numRead);
        }
    }
    return true;
}

bool SocketManager::HandleCommandSocketRead(IGenericSocket* socket)
{
    Log(LogSeverity::Verbose, "Command Socket");
    auto msg = AsyncMessage_t{};
    AsyncMessenger::Instance().ReadMessage(msg);

    if (msg.msgId == HOST_DETECT_RESULT) {
        auto* session = SessionManager::Instance().GetSession(msg.data.hostDetectResult.sessionId);
        Log(LogSeverity::Verbose, "HOST DETECT RESULT");
        if (msg.data.hostDetectResult.success == false) {
            Log(LogSeverity::Debug, "proxy detect aborted");
            const char* authMessage =
                    "HTTP/1.0 400 Client Error\n"
                    "Connection: close\r\n"
                    "\r\n";
            auto nwritten = ::write(session->GetClientFd(), authMessage, strlen(authMessage));
            ::close(session->GetClientFd());
            SessionManager::Instance().EndSession(msg.data.hostDetectResult.sessionId);
        } else {
            // Only try and do authentication if configuration requires it.
            if (AuthManager::Instance().IsEnabled()) {
                auto dupString = strdup(session->GetRequest().c_str());
                const auto* authString = strstr(dupString, "Proxy-Authorization: Basic ");
                if (authString != nullptr) {
                    char rawCreds[1024] = {};
                    auto canConnect = false;
                    auto rc = sscanf(authString, "Proxy-Authorization: Basic %1024s", rawCreds);
                    free(dupString);

                    auto userName = std::string{};
                    if (rc == 1) {
                        auto credString = std::string{rawCreds};

                        canConnect = AuthManager::Instance().Authenticate(credString, userName);

                        if (canConnect) {
                            session->SetUserName(userName);
                        }
                    }

                    if (canConnect) {
                        canConnect = AuthManager::Instance().AccessAllowedAtTime(userName);
                        if (!canConnect) {
                            Log(LogSeverity::Debug, "Rejecting connection due to time-based access controls");
                        }
                    }

                    if (!canConnect) {
                        const char* authMessage =
                                "HTTP/1.0 403 Forbidden\n"
                                "Connection: close\r\n"
                                "\r\n";
                        auto nwritten = ::write(session->GetClientFd(), authMessage, strlen(authMessage));
                        ::close(session->GetClientFd());
                        SessionManager::Instance().EndSession(msg.data.hostDetectResult.sessionId);
                        return true;
                    }
                } else {
                    free(dupString);

                    // Check IP Address - see if it's in whitelist.
                    auto& clientIp = session->GetClientAddress();
                    auto username = std::string{};
                    Log(LogSeverity::Debug, "Attempt authentication via IP");

                    if (AuthManager::Instance().AuthenticateIp(clientIp, username)) {
                        if (AuthManager::Instance().AccessAllowedAtTime(username)) {
                            Log(LogSeverity::Debug, "Authenticated as %s via IP", username.c_str());
                            session->SetUserName(username);
                        } else {
                            Log(LogSeverity::Debug, "Rejecting connection due to time-based access controls");
                            const char* authMessage =
                                    "HTTP/1.0 403 Forbidden\n"
                                    "Connection: close\r\n"
                                    "\r\n";
                            auto nwritten = ::write(session->GetClientFd(), authMessage, strlen(authMessage));
                            ::close(session->GetClientFd());
                            SessionManager::Instance().EndSession(msg.data.hostDetectResult.sessionId);
                            return true;
                        }
                    } else {
                        Log(LogSeverity::Debug, "Not authenticated - force retry", session->GetRequest().c_str());
                        const char* authMessage =
                                "HTTP/1.0 407 Proxy Authentication Required\r\n"
                                "Server: nermal\r\n"
                                "Content-Type: text/html\r\n"
                                "Proxy-Authenticate: Basic realm=\"Nermal\"\r\n"
                                "Connection: close\r\n"
                                "\r\n";
                        auto nwritten = ::write(session->GetClientFd(), authMessage, strlen(authMessage));
                        ::close(session->GetClientFd());
                        SessionManager::Instance().EndSession(msg.data.hostDetectResult.sessionId);
                        return true;
                    }
                }
            }            

            m_connector->ConnectProxy(msg.data.hostDetectResult.sessionId);
        }
    } else if (msg.msgId == HOST_CONNECT_RESULT) {
        Log(LogSeverity::Verbose, "HOST CONNECT RESULT");
        auto* session = SessionManager::Instance().GetSession(msg.data.hostDetectResult.sessionId);
        if (msg.data.hostConnectResult.success == false) {
            Log(LogSeverity::Debug, "proxy connect aborted, closing %d", session->GetSessionId());
            const char* authMessage =
                    "HTTP/1.0 404 Not Found\n"
                    "Connection: close\r\n"
                    "\r\n";
            auto nwritten = ::write(session->GetClientFd(), authMessage, strlen(authMessage));
            ::close(session->GetClientFd());
            SessionManager::Instance().EndSession(session->GetSessionId());
        } else {
            // Create Client and proxy objects
            auto clientFd = session->GetClientFd();
            auto proxyFd = msg.data.hostConnectResult.proxyFd;
            auto sessionId = msg.data.hostConnectResult.sessionId;

            Log(LogSeverity::Debug, "Session %d - New proxy fd: %d->%d  connected", sessionId, clientFd, proxyFd);

            auto newClientSocket = std::make_unique<ClientSocket>(clientFd, proxyFd, sessionId, true);
            AddSocket(std::move(newClientSocket));

            auto newProxySocket = std::make_unique<ClientSocket>(proxyFd, clientFd, sessionId, false);
            AddSocket(std::move(newProxySocket));
        }
    }
    return true;
}

bool SocketManager::ProcessSockets() {
    if (!m_isActive) {
        return false;
    }

    epoll_event events[m_eventsToProcess] = {};
    auto rc = epoll_wait(m_epollFd, events, m_eventsToProcess, m_epollTimeout);

    if (rc == -1) {
        if ((errno == EAGAIN) || (errno == EINTR)) {
            return true;
        }
        return false;
    }

    if (!rc) {
        PruneExcessConnections();
        if (GlobalStats::Instance().ReadyToLog()) {
            GlobalStats::Instance().LogAndReset();
        }
        return true;
    }

    for (auto i = 0; i < rc; i++) {
        auto sDone = false;
        for (auto& socket : m_sockets) {
            if (sDone) {
                break;
            }

            if (socket->GetFd() == events[i].data.fd) {
                switch (socket->Identity()) {
                    //-------------------------------------------------------------
                    case SocketType::Server: {
                        auto rc = HandleServerSocketRead(socket.get());
                        if (!rc) {
                            return false;
                        }
                    } sDone = true; break;
                    //-------------------------------------------------------------
                    case SocketType::Client: {                        
                        auto rc = HandleClientSocketRead(socket.get());
                        if (!rc) {
                            return false;
                        }
                    } sDone = true; break;
                    //-------------------------------------------------------------
                    case SocketType::Command: {
                        auto rc = HandleCommandSocketRead(socket.get());
                        if (!rc) {
                            return false;
                        }
                    } sDone = true; break;
                    default: {
                        Log(LogSeverity::Debug, "Unknown Socket");
                    } sDone = true; break;
                }
            }
        }
    }
    return true;
}
