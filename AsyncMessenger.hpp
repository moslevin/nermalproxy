#pragma once

#include <cstdint>
#include <mutex>

#include <unistd.h>

typedef enum {
    HOST_DETECT_RESULT,
    HOST_CONNECT_RESULT,
} AsyncMessageId_t;

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

class AsyncMessenger {
public:
    ~AsyncMessenger();

    static AsyncMessenger& Instance() {
        static AsyncMessenger* instance = new AsyncMessenger();
        return *instance;
    }

    int GetReadFd();
    bool WriteMessage(const AsyncMessage_t& message);
    void ReadMessage(AsyncMessage_t& message);

private:
    AsyncMessenger();

    using LockGuard = std::unique_lock<std::mutex>;

    static constexpr auto readPipe = 0;
    static constexpr auto writePipe = 1;

    int m_pipeFds[2];
    std::mutex m_writeLock;
};
