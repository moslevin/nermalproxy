# NermalProxy - a lightweight content filtering proxy
Copyright (C) 2019 m0slevin

## What is NermalProxy?

NermalProxy is a simple, policy-based, content-filtering HTTP 1.1 web proxy, written in modern C++14.  It filters ads (ala PiHole), 
blocks undesired content, limits access to certain times, logs usage statistics, and (of course) proxies data between a client device 
and the internet.

NermalProxy is released under the terms of the 3-clause BSD license.

## Why NermalProxy over PiHole / other proxies

Compared with other solutions, NermalProxy has a number of advantages.

- It's lightweight and fast - the server runs on very modest hardware, and performs well on single-core targets
- It's portable - there are no external dependencies (aside from pthreads and the C++ standard library)
- It has a simple text-based configuration format that's easy to use and extend
- It's purpose driven for ad-blocking and content-filtering in a home environment
- Permissive BSD licensing 
.

## What can NermalProxy do?

NermalProxy currently supports the following features:
- Block ads and other unwanted content via Blacklists from any device using the proxy - be it a phone, tablet, game console, or desktop
- Support for single host-per-line domain Blacklists
- Supports "global" and "user-specific" policies, allowing some users to access content restricted to others
- Supports both credential-based and IP-mapped authentication for proxy services
- Configurable audit logging for individual users of the proxy
- Supports HTTPS, HTTP, and other related services supporting HTTP 1.1 proxying 
- Configurable network access scheduling per-user
- Support "guest" internet access profiles
- Proxy internet connections from any ethernet devices connected to the host or subnet - wifi, lan, BT, BLE
- Runs on low-end linux-based targets, such as Raspberry Pi
.

... And more!

## Who is it for?

NermalProxy was designed with two primary use cases in mind.  
- Individuals or organizations looking for a simple device-agnostic solution for ad + content filtering across a wide-range of devices
- Families looking to provide content-filter and/or internet access/monitoring policies for different members of a household
- Individuals or organizations looking to provide a limited "guest" internet access profile on their netwworks
.

## How does it work?

NermalProxy implements an HTTP 1.1 tunneling proxy server, supporting both unencrypted HTTP and SSL HTTPS connections.

Upon startup, NermalProxy loads its configuration file containing policy data for all users of the network, and domain blacklists associated with the policy
configuration data are loaded into memory.  It then opens a proxy server (registered on all interfaces supported by the device), and listens for incoming
connections made by clients on the network.

Upon receiving an incoming network connection request on the regiserted server port, NermalProxy will accept the connection and read the client request 
data headers sent as part of the HTTP 1.1 tunneling request.  The client's IP address and/or credential data is checked against the loaded policy, and 
if the request is permitted, the server will attempt to resolve the address corresponding to the request.  If the destination address is resolved 
successfully, the proxy server will create a connection to the server on behalf of the client.  The client->proxy and proxy->open-internet connections are
mapped together within NermalProxy, with data trasparently proxied between them.  

Internally, NermalProxy tracks various statistics for each proxy session, and based on the policy, may create audit logs on a per-user basis.  These logs are
dumped periodically (daily) for users of the network.

## Threading Model

NermalProxy synchronizes data transfers between all connected sockets from a single dispatch thread, that blocks waiting for readible data on any
of its registered sockets.  This thread is used not only to synchronize client/server proxy data operations, but also handles internal message data
sent through pipes.  All socket operations and data transmissions take place on this thread, greatly simplifying the locking and synchronizing requirements
of the program.  

DNS and connection requests are sent to a threadpool for parallel processing, allowing for a large number of DNS resolutions to take place concurrently 
(with the remainder queued). These request/response handling operations are the only multi-threaded parts or the program - results are synchronized back
to the main application thread via a pipe-based message-passng scheme.

DNS results are further cached in-memory for a few minutes after use, preventing unnecessary DNS requests for recently-used services.  There is also a simple
(and primative) LRU cache implemented in this code.  

## How to build it?

Ensure you have a recent-ish CMake and a C++14 toolchain installed.  While tested on various flavors of Linux, any *nix like OS should work just fine.

$ mkdir kbuild
$ cd kbuild
$ cmake ..

## How to run it?

./nermalproxy {path to conf file}

if path to configuration file is not specified, a file named "default.conf" is loaded instead.  see the provided "default.conf" for documentation of the
various program options and policy settings supported by NermalProxy.

Ideally, the service will be launched at startup via a shell script.
