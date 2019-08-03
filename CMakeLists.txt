project(nermalproxy)

set(TARGET_CXX_FLAGS "-Os -Og -g3")
set(TARGET_LINKER_FLAGS "-pthread")

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${TARGET_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${TARGET_LINKER_FLAGS}")

set(TARGET_SRC
    AsyncMessenger.cpp
    BlackList.cpp
    ClientSocket.cpp
    CommandSocket.cpp
    ConfigFile.cpp
    HostInfoManager.cpp
    HostResolver.cpp
    Log.cpp
    main.cpp
    Policy.cpp
    ProxyConnector.cpp
    ServerSocket.cpp
    Session.cpp
    SocketManager.cpp
    Timestamp.cpp
    UserAuth.cpp
    )

set(TARGET_INCLUDE
    AsyncMessenger.hpp
    BlackList.hpp
    ClientSocket.hpp
    CommandSocket.hpp
    ConfigFile.hpp
    HostInfoManager.hpp
    IGenericSocket.hpp
    IHostResolver.hpp
    Log.hpp
    ProxyConnector.hpp
    ServerSocket.hpp
    Session.hpp
    SocketManager.hpp
    Timestamp.hpp
    UserAuth.hpp
    )

add_executable(nermalproxy ${TARGET_SRC} ${TARGET_INCLUDE})

