#include "ProxyConnector.hpp"

#include <arpa/inet.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <mutex>
#include <semaphore.h>
#include <thread>

#include "AsyncMessenger.hpp"
#include "HostResolver.hpp"
#include "Session.hpp"
#include "UserAuth.hpp"
#include "Log.hpp"
#include "Blacklist.hpp"
#include "Policy.hpp"

using ThreadFunc = void (*)(void* context);

typedef struct {
    ThreadFunc handler;
    void* context;
} WorkPackage;

class ThreadPool {
public:
    static ThreadPool& Instance() {
        static ThreadPool* instance= new ThreadPool{};
        return *instance;
    }

    void Begin() {
        auto workerTask = [&]() {
            while(1) {
                sem_wait(&m_poolSem);
                auto package= WorkPackage{};
                {
                    auto lg = LockGuard{m_workMutex};
                    package = m_work.front();
                    m_work.pop_front();
                }
                if (package.handler) {
                    package.handler(package.context);
                }
            }
        };

        for (auto i = 0; i < threadPoolSize; i++) {
            auto* newThread = new std::thread(workerTask);
            newThread->detach();
        }
    }

    void Dispatch(WorkPackage& package) {
        auto lg = LockGuard{m_workMutex};
        m_work.push_back(package);
        sem_post(&m_poolSem);
    }

private:
    ThreadPool()
    {
        sem_init(&m_poolSem, 0, 0);
        Begin();
    }

    static constexpr auto threadPoolSize = 20; // threads

    using LockGuard = std::unique_lock<std::mutex>;
    sem_t m_poolSem;

    std::mutex m_workMutex;
    std::list<WorkPackage> m_work;
};

ProxyConnector::ProxyConnector()
{}

void ProxyConnector::BeginProxyDetect(const int sessionId) {
    // Figure out what kind of proxy connection this is...

    struct proxyDeets {
        int sessionId;
        void cleanup() {
            delete this;
        }
    };

    static auto asyncDetector = [](void* ctx) {
        Log(LogSeverity::Debug, "asyncDetectord:enter", __func__);
        auto* deets = static_cast<struct proxyDeets*>(ctx);
        int sessionId = deets->sessionId;
        auto* session = SessionManager::Instance().GetSession(sessionId);
        int clientFd = session->GetClientFd();

        char buf[4096] = {};
        auto transparent = false;
        auto numRead = ::read(clientFd, buf, sizeof(buf));
        auto success = true;
        auto port = int{80};
        char url[1024] = {};

        if (numRead <= 0) {
            Log(LogSeverity::Debug, "Bail on read - fd=%d, errno%d", clientFd, errno);
            success = false;
        } else {

            // Parse out the connect string...
            char mode[1024] = {};
            auto numScanned = ::sscanf(buf, "%1024s %1024s HTTP/1.%*d", mode, url);

            if (0 == strcmp("CONNECT", mode)) {
                Log(LogSeverity::Debug, "fd=%d connect-mode proxy", clientFd);
                transparent = false;
            } else if (0 == strcmp("GET", mode)) {
                Log(LogSeverity::Debug, "fd=%d get-mode proxy", clientFd);
                transparent = true;
            } else if (0 == strcmp("POST", mode)) {
                Log(LogSeverity::Debug, "fd=%d post-mode proxy", clientFd);
                transparent = true;
            }

            if (!transparent) {
                // Parse out the port from the url
                auto* lastColon = strchr(url, ':');
                if (lastColon) {
                    ::sscanf(lastColon, ":%d", &port);
                    *lastColon = '\0';
                }
            } else {
                // Support transparent mode proxy for HTTP traffic only (GET/POST)
                numScanned = ::sscanf(buf, "%*s http://%1024s HTTP/1.%*d", url);
                if (numScanned != 1) {
                    Log(LogSeverity::Debug, "Invalid proxy connect string - %s", buf);
                    success = false;
                } else {
                    // Parse out any trailing slashes
                    auto* lastSlash = strchr(url, '/');
                    if (lastSlash) {
                        *lastSlash = '\0';
                    }
                }
            }
        }

        auto urlString = std::string(url);
        auto requestString = std::string(buf);
        if (success) {
            session->SetHost(urlString);
            session->SetPort(port);
            session->SetRequest(requestString);
            session->SetTransparent(transparent);
        }

        auto msg = AsyncMessage_t{};
        msg.msgId = HOST_DETECT_RESULT;
        msg.data.hostDetectResult.sessionId = sessionId;
        msg.data.hostDetectResult.success = success;

        deets->cleanup();
        AsyncMessenger::Instance().WriteMessage(msg);                
    };

    auto* deets = new proxyDeets{.sessionId = sessionId};
    auto work = WorkPackage{.handler = asyncDetector, .context = deets};

    ThreadPool::Instance().Dispatch(work);
}

void ProxyConnector::ConnectProxy(const int sessionId) {
    // Host lookup + connect...
    // ToDo: Do this in a threadpool to keep it from blowing up...

    struct connectDeets {
        int sessionId;
        void cleanup() {
            delete this;
        }
    };

    static auto asyncConnector = [](void* ctx) {
        Log(LogSeverity::Debug, "asyncConnector:enter", __func__);

        auto* deets = static_cast<struct connectDeets*>(ctx);
        int sessionId = deets->sessionId;        
        auto* session = SessionManager::Instance().GetSession(sessionId);

        int idx = 0;
        bool allowed = true;
        std::string domain;

        while (UserPolicyList::Instance().GetUserDomain(idx++, session->GetUserName(), domain)) {
            if (!BlacklistList::Instance().IsAllowedInList(domain, session->GetHost())) {
                Log(LogSeverity::Debug, "host %s not allowed in domain %s", session->GetHost().c_str(), domain.c_str());
                allowed = false;
                break;
            }
        }

        idx = 0;
        if (allowed) {
            auto globalDomain = std::string{"<global>"};
            while (UserPolicyList::Instance().GetUserDomain(idx++, globalDomain, domain)) {
                if (!BlacklistList::Instance().IsAllowedInList(domain, session->GetHost())) {
                    Log(LogSeverity::Debug, "host %s not allowed in domain %s", session->GetHost().c_str(), domain.c_str());
                    allowed = false;
                    break;
                }
            }
        }

        if (!allowed) {
            // Send an asynchronous message back to the main thread terminating the session....
            auto msg = AsyncMessage_t{};
            msg.msgId = HOST_CONNECT_RESULT;
            msg.data.hostConnectResult.proxyFd = -1;
            msg.data.hostConnectResult.sessionId = sessionId;
            msg.data.hostConnectResult.success = false;
            AsyncMessenger::Instance().WriteMessage(msg);
            deets->cleanup();
            return;

        }

        auto cached = false;
        auto hostAddresses = HostResolver::Instance().GetAddressesForHost(session->GetHost(), session->GetPort(), cached);
        auto sockFd = ::socket(AF_INET, SOCK_STREAM, 0);
        auto connected = false;

        if (sockFd != -1) {
            auto rc = int{-1};
            for (auto& address : hostAddresses) {
                auto dest = (struct sockaddr_in){};
                auto size = size_t{};

                // Address being filtered -- ignore.
                if (address.url == "0.0.0.0") {
                    continue;
                }

                Log(LogSeverity::Debug, "trying to connect to %s via %s:%d", address.url.c_str(), address.address.c_str(), session->GetPort());
                if (address.ipVersion == 4) {
                    auto* dest4 = reinterpret_cast<struct sockaddr_in*>(&dest);
                    dest4->sin_family = AF_INET;
                    dest4->sin_port = htons(static_cast<uint16_t>(session->GetPort()));
                    size = sizeof(sockaddr_in);

                    if (0 == inet_pton(AF_INET, address.address.c_str(), &(dest4->sin_addr))) {
                        Log(LogSeverity::Debug, "Can't convert address");
                        continue;
                    }
                } else {
                    auto* dest6 = reinterpret_cast<struct sockaddr_in6*>(&dest);
                    dest6->sin6_family = AF_INET6;
                    dest6->sin6_port = htons(static_cast<uint16_t>(session->GetPort()));
                    size = sizeof(sockaddr_in6);

                    if (0 == inet_pton(AF_INET6, address.address.c_str(), &(dest6->sin6_addr))) {
                        Log(LogSeverity::Debug, "Can't convert address");
                        continue;
                    }
                }
                rc = ::connect(sockFd, (const sockaddr*)&dest, sizeof(sockaddr_in6));

                if (rc != -1) {
                    Log(LogSeverity::Debug, "Connected");
                    break;
                }
            }

            if (rc == -1) {
                Log(LogSeverity::Debug, "Couldn't connect");
            } else {
                connected = true;
                if (session->GetTransparent()) {

                    // GET mode proxying is assumed to be transparent - resend the request.
                    auto rc = ::write(sockFd, session->GetRequest().c_str(), session->GetRequest().length());
                    if (rc == -1) {
                        Log(LogSeverity::Error, "%s: Unable to resend initial request", __func__);
                    }
                } else {
                    static const char* responseString = "HTTP/1.0 200 OK\r\n\r\n";
                    auto rc = ::write(session->GetClientFd(), responseString, strlen(responseString));
                    if (rc == -1) {
                        Log(LogSeverity::Error, "%s: Unable to write success code", __func__);
                    }
                }
            }
        }

        // Parse out headers...
        if (connected) {
            auto alive = int{1};
            if (0 == strstr(session->GetRequest().c_str(), "Connection: keep-alive")) {
                setsockopt(sockFd, SOL_SOCKET, SO_KEEPALIVE, &alive, sizeof(alive));
            }
            if (0 == strstr(session->GetRequest().c_str(), "Proxy-Connection: keep-alive")) {
                setsockopt(session->GetClientFd(), SOL_SOCKET, SO_KEEPALIVE, &alive, sizeof(alive));
            }
        }

        // Send an asynchronous message back to the main thread
        auto msg = AsyncMessage_t{};
        msg.msgId = HOST_CONNECT_RESULT;
        msg.data.hostConnectResult.proxyFd = sockFd;
        msg.data.hostConnectResult.sessionId = sessionId;
        msg.data.hostConnectResult.success = connected;

        if (!connected) {
            HostResolver::Instance().ClearCacheForHost(session->GetHost());
            ::close(sockFd);
        }

        AsyncMessenger::Instance().WriteMessage(msg);
        deets->cleanup();
    };

    auto* deets = new connectDeets{};
    deets->sessionId = sessionId;

    auto work = WorkPackage{.handler = asyncConnector, .context = deets};

    ThreadPool::Instance().Dispatch(work);
}
