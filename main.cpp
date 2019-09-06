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

    signal(SIGHUP, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    // ToDo -- chdir + umask?

    for (auto i = 0; i < sysconf(_SC_OPEN_MAX); i++) {
        close(i);
    }
    stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");
}

void DoConfig() {
    auto config = ConfigFile{"test.conf"};
    config.LoadData();

    // Load general proxy settings
    auto proxyConfig = config.GetSection("Proxy");

    auto daemonize = proxyConfig.GetAttribute("daemon_mode");
    if (daemonize.GetValue() == "enabled") {
        Daemonize();
    }

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

    auto& logToFile = proxyConfig.GetAttribute("log_to_file");
    if (logToFile.GetValue() == "enabled") {
        Log(LogSeverity::Debug, "Store logs to file enabled");
        LogStoreToDisk(true);
    }

    auto& proxyPort = proxyConfig.GetAttribute("port");
    if (proxyPort.GetValue() != "") {
        g_serverPort = std::uint16_t(std::stoul(proxyPort.GetValue()));
        Log(LogSeverity::Debug, "Setting proxy port=%d", g_serverPort);
    }
    auto& proxyAuthEnabled = proxyConfig.GetAttribute("auth");
    if (proxyAuthEnabled.GetValue() == "enabled") {
        Log(LogSeverity::Debug, "Enabling proxy authentication");
        g_authEnabled = true;
    }

    // Load the domain files first...
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

            auto& ipAttr = policyConfig.GetAttribute("ip_list");
            auto ipValue = std::string{};
            auto ipIdx = 0;
            while (ipAttr.GetValue(ipIdx++, ipValue)) {
                AuthManager::Instance().AddUserIp(attr->GetName(), ipValue);
            }

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

    // Add the default policy...
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

int main(void)
{
    DoConfig();

    auto proxyConnector = std::make_unique<ProxyConnector>();
    auto socketManager = std::make_unique<SocketManager>(std::move(proxyConnector));
    if (!socketManager->Initialize()) {
        Log(LogSeverity::Error, "Failed to initialize socketmanager");
        return -1;
    }

    auto server = std::make_unique<ServerSocket>();
    if (!server->Initialize(g_serverPort)) {
        Log(LogSeverity::Error, "Failed to initialize server");
        return -1;
    }
    socketManager->AddSocket(std::move(server));

    auto commandSocket = std::make_unique<CommandSocket>();
    socketManager->AddSocket(std::move(commandSocket));

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, 0) == -1) {
        Log(LogSeverity::Error, "Failed to set SIGIGN on SIGPIPE");
        return -1;
    }

    while (socketManager->ProcessSockets()) {
        // Do nothing
    }

    Log(LogSeverity::Error, "I'm sorry, Jon...");
    return 0;
}
