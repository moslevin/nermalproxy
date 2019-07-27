#include "SocketManager.hpp"

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

bool SocketManager::ProcessSockets() {
    if (!m_isActive) {
        return false;
    }

    epoll_event events[10];
    auto rc = epoll_wait(m_epollFd, events, 10, 100);

    if (rc == -1) {
        if ((errno == EAGAIN) || (errno == EINTR)) {
            return true;
        }
        return false;
    }

    if (!rc) {
        while (m_sockets.size() > 100) {
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
                        auto* server = static_cast<ServerSocket*>(socket.get());
                        auto clientIp = std::string{};

                        auto clientFd = server->Accept(clientIp);
                        if (clientFd < 0) {
                            Log(LogSeverity::Debug, "Error accepting client");                            
                            return false;
                        }

                        // Check to see if clientIp has an active user session...
                        auto* session = SessionManager::Instance().CreateSession(clientFd);
                        session->SetClientAddress(clientIp);
                        Log(LogSeverity::Debug, "New Client fd=%d -- async proxy detection", clientFd);
                        m_connector->BeginProxyDetect(session->GetSessionId());
                    } sDone = true; break;
                    //-------------------------------------------------------------
                    case SocketType::Client: {
                        auto* client = static_cast<ClientSocket*>(socket.get());
                        static std::uint8_t buf[32768];
                        auto error = false;
                        auto numRead = client->Read(buf, sizeof(buf), &error);
                        auto sessionId = client->GetSessionId();
                        if (error) {
                            Log(LogSeverity::Verbose, "Session: %d Destroying Client Socket ", client->GetSessionId());
    
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
                        } else {
                            auto numWritten = client->Write(buf, numRead, &error);

                            auto* session = SessionManager::Instance().GetSession(sessionId);
                            if (client->IsClientToProxy()) {
                                session->AddRxBytes(numWritten);
                            } else {
                                session->AddTxBytes(numRead);
                            }
                        }
                    } sDone = true; break;
                    //-------------------------------------------------------------
                    case SocketType::Command: {
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

                                        if (rc == 1) {
                                            auto credString = std::string{rawCreds};
                                            auto userName = std::string{};

                                            canConnect = AuthManager::Instance().Authenticate(credString, userName);
                                            if (canConnect) {
                                                session->SetUserName(userName);
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
                                            sDone = true; break;
                                        }
                                    } else {
                                        free(dupString);

                                        // Check IP Address - see if it's in whitelist.
                                        auto& clientIp = session->GetClientAddress();
                                        auto username = std::string{};
                                        if (AuthManager::Instance().AuthenticateIp(clientIp, username)) {
                                            Log(LogSeverity::Debug, "Authenticated as %s via IP", username.c_str());
                                            session->SetUserName(username);
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
                                            sDone = true; break;
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
