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

#pragma once

#include <list>
#include <memory>

#include "IGenericSocket.hpp"
#include "ProxyConnector.hpp"

/**
 * @brief The SocketManager class manages the lifecycle of all socket
 * connections (client/server/proxy/internal-ipc) managed by the main
 * event loop.
 */
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

