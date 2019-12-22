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

#include "IGenericSocket.hpp"

#include <cstddef>
#include <cstdint>

/**
 * @brief The ClientSocket class represents a connection between the proxy and a client or server.
 */
class ClientSocket : public IGenericSocket
{
public:
    ClientSocket(const int socketFd, const int proxyFd, const int sessionId, const bool clientToProxy);
    virtual ~ClientSocket();

    // Get the fd representing the connection
    int GetFd() const override;

    // Get the enum representing the "type" of socket implemented in this object
    SocketType Identity() const override;

    // Get the unique session identifier corresponding to this socket
    int GetSessionId();

    // Read data from the socket (consume data written by client)
    std::size_t Read(void* readBuf_, std::size_t bytesToRead_, bool* isError_);

    // Write data to the socket (send data to the client)
    std::size_t Write(const void* writeBuf_, std::size_t bytesToWrite_, bool* isError_);

    // Get the fd representing the remote target (server) to which we're proxying
    // the conneciton
    int GetProxyFd();    

    // returns whether or not the connecdtion represents a client-to-proxy channel
    // (as opposed to a target-to-proxy channel)
    bool IsClientToProxy();

private:
    void OnError();

    bool m_isActive;
    bool m_isClientToProxy;
    int m_socketFd;
    int m_peerFd;
    int m_sessionId;
};
