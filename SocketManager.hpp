#pragma once

#include <list>
#include <memory>

#include "IGenericSocket.hpp"
#include "ProxyConnector.hpp"

class SocketManager {
public:
    SocketManager(std::unique_ptr<ProxyConnector> connector);
    ~SocketManager() = default;

    bool Initialize();
    bool AddSocket(std::unique_ptr<IGenericSocket> genericSocket);
    bool RemoveSocketFd(int fd);
    bool ProcessSockets();

private:
    static constexpr auto m_maxConcurrentConnections = 256;
    static constexpr auto m_eventsToProcess = 10;
    static constexpr auto m_epollTimeout = 100; // ms

    void PruneExcessConnections();
    bool HandleServerSocketRead(IGenericSocket* socket);
    bool HandleClientSocketRead(IGenericSocket* socket);
    bool HandleCommandSocketRead(IGenericSocket* socket);

    std::list<std::unique_ptr<IGenericSocket>> m_sockets;

    int m_epollFd;
    bool m_isActive;
    std::unique_ptr<ProxyConnector> m_connector;
};

