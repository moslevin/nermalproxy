#pragma once

#include <cstdint>

enum class SocketType : std::uint8_t {
    Server,
    Client,
    Command,
};

class IGenericSocket {
public:
    virtual ~IGenericSocket() = default;

    virtual int GetFd() const = 0;
    virtual SocketType Identity() const = 0;
};
