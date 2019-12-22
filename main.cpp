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

#include <memory>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "AsyncMessenger.hpp"
#include "BlackList.hpp"
#include "ClientSocket.hpp"
#include "CommandSocket.hpp"
#include "HostInfoManager.hpp"
#include "HostResolver.hpp"
#include "Log.hpp"
#include "ProxyConnector.hpp"
#include "ServerSocket.hpp"
#include "SocketManager.hpp"
#include "Timestamp.hpp"
#include "UserAuth.hpp"
#include "BlackList.hpp"
#include "ConfigFile.hpp"
#include "Policy.hpp"
#include "TimeMap.hpp"

namespace {

std::uint16_t g_serverPort = 10080;
bool g_authEnabled = false;

void Daemonize() {
    pid_t newPid = fork();
    if (newPid < 0) {
        Log(LogSeverity::Error, "Failed to fork child process");
        exit(-1);
    }
    if (newPid > 0) {
        Log(LogSeverity::Info, "Terminating parent process - Daemon spawned");
        exit(0);
    }
    if (setsid() < 0) {
        Log(LogSeverity::Error, "Unable to setsid for child process");
        exit(-1);
    }

    // Ignore signals that would otherwise cause us exit.
    signal(SIGHUP, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    // ToDo -- chdir + umask?

    // Close all open file handles
    for (auto i = 0; i < sysconf(_SC_OPEN_MAX); i++) {
        close(i);
    }

    // Re-open stdin/stdout/stderr as /dev/null
    stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");
}

void DoConfig(const std::string& filename) {
    auto config = ConfigFile{filename.c_str()};
    config.LoadData();

    // Load general proxy settings
    auto proxyConfig = config.GetSection("Proxy");

    // If the deamon_mode option is specified, daemonize the process immediately.
    auto daemonize = proxyConfig.GetAttribute("daemon_mode");
    if (daemonize.GetValue() == "enabled") {
        Daemonize();
    }

    // Check for global logging verbosity option and apply
    auto& logVerbosity = proxyConfig.GetAttribute("log_verbosity");
    if (logVerbosity.GetValue() != "") {
        if (logVerbosity.GetValue() == "debug") {
            LogSetVerbosity(LogSeverity::Debug);
        } else if (logVerbosity.GetValue() == "info") {
            LogSetVerbosity(LogSeverity::Info);
        } else if (logVerbosity.GetValue() == "warn") {
            LogSetVerbosity(LogSeverity::Warn);
        } else if (logVerbosity.GetValue() == "error") {
            LogSetVerbosity(LogSeverity::Error);
        } else {
            Log(LogSeverity::Error, "Invalid verbosity =%s", logVerbosity.GetValue().c_str());
        }
    }

    // Enable log-redirection to file
    auto& logToFile = proxyConfig.GetAttribute("log_to_file");
    if (logToFile.GetValue() == "enabled") {
        Log(LogSeverity::Debug, "Store logs to file enabled");
        LogStoreToDisk(true);
    }

    // Override default proxy port
    auto& proxyPort = proxyConfig.GetAttribute("port");
    if (proxyPort.GetValue() != "") {
        g_serverPort = std::uint16_t(std::stoul(proxyPort.GetValue()));
        Log(LogSeverity::Debug, "Setting proxy port=%d", g_serverPort);
    }

    // Enable authentication by IP or user/password
    auto& proxyAuthEnabled = proxyConfig.GetAttribute("auth");
    if (proxyAuthEnabled.GetValue() == "enabled") {
        Log(LogSeverity::Debug, "Enabling proxy authentication");
        g_authEnabled = true;
    }

    // Load the domain-filtering files
    auto domainConfig = config.GetSection("DomainFiles");
    MultiValueAttribute* attr = nullptr;
    auto idx = 0;
    while ((attr = domainConfig.GetAttribute(idx++)) != nullptr) {
        Log(LogSeverity::Debug, "Adding domain... %s", attr->GetName().c_str());
        auto domain = std::make_unique<Blacklist>(attr->GetName());
        if (domain->LoadFromFile(attr->GetValue())) {
            BlacklistList::Instance().AddList(std::move(domain));
        }
    }

    // Process list of users (and their passwords) if authentication is enabled
    if (g_authEnabled) {
        AuthManager::Instance().SetEnabled(true);

        auto authData = config.GetSection("UserAuth");
        attr = nullptr;
        idx = 0;
        while ((attr = authData.GetAttribute(idx++)) != nullptr) {
            Log(LogSeverity::Debug, "Adding user: %s", attr->GetName().c_str());
            AuthManager::Instance().AddUser(attr->GetName(), attr->GetValue());

            // Create a policy for each user - stored in a section named after the user
            auto policy = std::make_unique<UserPolicy>(attr->GetName());
            auto policyConfig = config.GetSection(attr->GetName());

            // Parse all domains in the config file for policy
            auto domainIdx = 0;
            auto& domainAttr = policyConfig.GetAttribute("domains");
            auto domainValue = std::string{};
            while (domainAttr.GetValue(domainIdx++, domainValue)) {
                if (BlacklistList::Instance().HasList(domainValue)) {
                    Log(LogSeverity::Debug, "Adding domain: %s", domainValue.c_str());
                    policy->AddDomain(domainValue);
                } else {
                    Log(LogSeverity::Warn, "Ignoring unknown domain: %s", domainValue.c_str());
                }
            }

            // Specify IP addresses corresponding to the specific user
            auto& ipAttr = policyConfig.GetAttribute("ip_list");
            auto ipValue = std::string{};
            auto ipIdx = 0;
            while (ipAttr.GetValue(ipIdx++, ipValue)) {
                AuthManager::Instance().AddUserIp(attr->GetName(), ipValue);
            }

            // enable audit logging for the user
            auto& auditAttr = policyConfig.GetAttribute("audit");
            if (auditAttr.GetValue() == "enabled") {
                Log(LogSeverity::Debug, "Auditing enabled for user: %s", attr->GetName().c_str());
                AuthManager::Instance().SetAudit(attr->GetName(), true);
            }

            // See if there are time-based access policies configured for the user
            auto& timeAccessAttr = policyConfig.GetAttribute("time_based_policy");
            if (timeAccessAttr.GetValue() == "enabled") {
                Log(LogSeverity::Debug, "Enabling time-based access policy for user %s", attr->GetName().c_str());

                std::vector<std::string> dayStrings = {"sun", "mon", "tue", "wed", "thr", "fri", "sat"};

                for (auto&& day : dayStrings) {
                    auto& dayAccessAttr = policyConfig.GetAttribute(day);
                    Log(LogSeverity::Debug, "Found policy for day %s: %s", day.c_str(), dayAccessAttr.GetValue().c_str());

                    if (dayAccessAttr.GetValue() != "") {
                        AuthManager::Instance().SetWeeklyAccess(attr->GetName(), day, dayAccessAttr.GetValue());
                    }
                }
            }
            // Move policy to the global policy list.
            UserPolicyList::Instance().AddPolicy(std::move(policy));
        }
    }

    // Add the default policy that is applied to users
    auto& globalDomains = proxyConfig.GetAttribute("global_domains");
    auto globalPolicy = std::make_unique<UserPolicy>("<global>");
    auto globalDomainValue = std::string{};
    idx = 0;
    while (globalDomains.GetValue(idx++, globalDomainValue)) {
        Log(LogSeverity::Debug, "Adding global domain: %s", globalDomainValue.c_str());
        globalPolicy->AddDomain(globalDomainValue);
    }
    UserPolicyList::Instance().AddPolicy(std::move(globalPolicy));
}
} // anonymous namespace

int main(int argc, char** argv)
{
    auto configFile = std::string{"default.conf"};

    // user can override config file w/command-line argument, otherwise attempt to load
    // default.conf
    if (argc >= 2) {
        if (0 != access(argv[1], F_OK)) {
            fprintf(stderr, "config file %s does not exist\n", argv[1]);
            return -1;
        }
        configFile = std::string{argv[1]};
    }

    // Load config file (if config file is not loaded, use default config)
    DoConfig(configFile);

    // Create the objects required to manage the sockets
    auto proxyConnector = std::make_unique<ProxyConnector>();
    auto socketManager = std::make_unique<SocketManager>(std::move(proxyConnector));
    if (!socketManager->Initialize()) {
        Log(LogSeverity::Error, "Failed to initialize socketmanager");
        return -1;
    }

    // Create the object implementing the server (incoming connections from clients)
    auto server = std::make_unique<ServerSocket>();
    if (!server->Initialize(g_serverPort)) {
        Log(LogSeverity::Error, "Failed to initialize server");
        return -1;
    }
    socketManager->AddSocket(std::move(server));

    // Creat the object that handles local IPC requests/responses in a thread-safe way
    auto commandSocket = std::make_unique<CommandSocket>();
    socketManager->AddSocket(std::move(commandSocket));

    // Ignore broken pipe signals -- process exits otherwise...
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, 0) == -1) {
        Log(LogSeverity::Error, "Failed to set SIGIGN on SIGPIPE");
        return -1;
    }

    // Main processing loop for sockets -- won't return unless there's an error.
    while (socketManager->ProcessSockets()) {
        // Do nothing
    }

    Log(LogSeverity::Error, "I'm sorry, Jon...");
    return 0;
}
