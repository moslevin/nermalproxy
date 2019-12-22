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

#include <cstdint>
#include <mutex>

#include <unistd.h>

// enum defining the message types sent w/the AsyncMessenger
typedef enum {
    HOST_DETECT_RESULT,
    HOST_CONNECT_RESULT,
} AsyncMessageId_t;

// message struct for IPC data
typedef struct {
    AsyncMessageId_t msgId;
    union {
        struct __attribute__((packed)) {
            int sessionId;
            bool success;            
        } hostDetectResult;
        struct __attribute__((packed)) {            
            int sessionId;
            int proxyFd;
            bool success;            
        } hostConnectResult;
    } data;
} AsyncMessage_t;

/**
 * @brief The AsyncMessenger class is used to send messages asynchronously
 * within the process.
 */
class AsyncMessenger {
public:
    ~AsyncMessenger();

    // Singleton
    static AsyncMessenger& Instance() {
        static AsyncMessenger* instance = new AsyncMessenger();
        return *instance;
    }

    // fd from which the messages can be consumed
    int GetReadFd();

    // Write a message to the messenger object
    bool WriteMessage(const AsyncMessage_t& message);

    // Read a message from the messenger object
    void ReadMessage(AsyncMessage_t& message);

private:
    AsyncMessenger();

    using LockGuard = std::unique_lock<std::mutex>;

    static constexpr auto readPipe = 0;
    static constexpr auto writePipe = 1;

    int m_pipeFds[2];
    std::mutex m_writeLock;
};
