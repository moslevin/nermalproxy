#include "CommandSocket.hpp"

#include "AsyncMessenger.hpp"

int CommandSocket::GetFd() const {
    return AsyncMessenger::Instance().GetReadFd();
}

SocketType CommandSocket::Identity() const {
    return SocketType::Command;
}
