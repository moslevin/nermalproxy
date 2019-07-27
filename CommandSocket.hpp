#pragma once

#include "IGenericSocket.hpp"

class CommandSocket : public IGenericSocket
{
public:
    int GetFd() const override;
    SocketType Identity() const override;
};
