#pragma once
#include <string>
#include "IHostResolver.hpp"

class ProxyConnector {
public:
    ProxyConnector();

    void BeginProxyDetect(const int sessionId);
    void ConnectProxy(const int sessionId);

private:
    static constexpr auto maxAsyncTasks = 8;
};
