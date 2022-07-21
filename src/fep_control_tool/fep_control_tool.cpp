/**
 * @file
 * @copyright
 * @verbatim
Copyright @ 2021 VW Group. All rights reserved.

    This Source Code Form is subject to the terms of the Mozilla
    Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

If it is not possible or desirable to put the notice in a particular file, then
You may include the notice in a location (such as a LICENSE file in a
relevant directory) where a recipient would be likely to look for such a notice.

You may add additional accurate notices of copyright ownership.

@endverbatim
 */


#include "fep_control.h"
#include "fep_control_commandline.h"
#include "fep_control_websocket.h"

#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

namespace {
std::atomic<bool> shutdown_requested{false};
enum mode { COMMANDLINE, WEBSOCKET };
mode operation_mode = COMMANDLINE;
} // namespace

#ifdef __linux__

void signal_handler(int signal)
{
    shutdown_requested = true;
}

#elif _WIN32

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    // See https://docs.microsoft.com/en-us/windows/console/handlerroutine for signals
    BOOL signal_handled = FALSE;
    switch (fdwCtrlType) {
    // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
        // Avoid shutdown with CTRL+C shortcut in COMMANDLINE mode
        if (operation_mode == WEBSOCKET) {
            shutdown_requested = true;
        }
        signal_handled = TRUE;
        break;

    // CTRL-CLOSE: confirm that the user wants to exit.
    case CTRL_CLOSE_EVENT:
        shutdown_requested = true;
        signal_handled = TRUE;
        break;

    // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT:
        signal_handled = FALSE;
        break;

    case CTRL_LOGOFF_EVENT:
        signal_handled = FALSE;
        break;

    case CTRL_SHUTDOWN_EVENT:
        signal_handled = FALSE;
        break;

    default:
        signal_handled = FALSE;
        break;
    }
    // FIXME join threads and block instead of hardcoded timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return signal_handled;
}
#endif

class ActiveWebsocketConnections {
public:
    void addConnection(std::shared_ptr<FepControl> instance)
    {
        std::lock_guard<std::mutex> lck(_mutex_vector);
        active_connections.push_back(std::move(instance));
    }
    void removeConnection(std::shared_ptr<FepControl> instance)
    {
        std::lock_guard<std::mutex> lck(_mutex_vector);
        active_connections.erase(
            std::remove(active_connections.begin(), active_connections.end(), instance),
            active_connections.end());
    }
    std::size_t size() const
    {
        std::lock_guard<std::mutex> lck(_mutex_vector);
        return active_connections.size();
    }
    std::vector<std::shared_ptr<FepControl>> getConnections() const
    {
        std::lock_guard<std::mutex> lck(_mutex_vector);
        return active_connections;
    }

private:
    std::vector<std::shared_ptr<FepControl>> active_connections;
    mutable std::mutex _mutex_vector;
};
void interactiveLoopWebsocket(bool json_mode)
{
    std::shared_ptr<ActiveWebsocketConnections> active_websocket_connections =
        std::make_shared<ActiveWebsocketConnections>();

    std::thread t([json_mode, active_websocket_connections]() {
        try {
            const std::string address_string = "0.0.0.0";

            auto const address = boost::asio::ip::make_address(address_string);
            auto const port = static_cast<unsigned short>(9003);

            // The io_context is required for all I/O
            boost::asio::io_context ioc{1};

            // The acceptor receives incoming connections
            boost::asio::ip::tcp::acceptor acceptor{ioc, {address, port}};

            for (;;) {
                // This will receive the new connection
                boost::asio::ip::tcp::socket socket{ioc};

                // Block until we get a connection
                acceptor.accept(socket);

                std::shared_ptr<FepControl> instance =
                    std::make_shared<FepControlWebsocket>(std::move(socket), json_mode);

                active_websocket_connections->addConnection(instance);

                std::cout << "***A new Client has just connected. Start working now.***"
                          << std::endl;
                std::cout << "Active connections: " << active_websocket_connections->size()
                          << std::endl;
                std::thread t([instance, active_websocket_connections]() {
                    instance->readInputFromSource();
                    std::cout << "Remove Client from list of active connections." << std::endl;
                    active_websocket_connections->removeConnection(instance);
                    std::cout << "Active connections: " << active_websocket_connections->size()
                              << std::endl;
                });
                t.detach();
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error in __FUNCTION__:__LINE__: " << e.what() << "\n";
        }
    });
    t.detach();

    while (!shutdown_requested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "***Shutdown requested from user. Active connections: "
              << active_websocket_connections->size() << std::endl;
    for (const auto& instance: active_websocket_connections->getConnections()) {
        std::cout << "Write good bye message to Client." << std::endl;
        instance->writeShutdownMessage();
    }
    std::cout << "Terminating application." << std::endl;
}

void interactiveLoopCLI(bool json_mode)
{
    std::vector<std::shared_ptr<FepControl>> instances;

    // listen for user input in command line mode
    // Create CLI object
    std::shared_ptr<FepControl> instance = std::make_shared<FepControlCommandLine>(json_mode);
    instances.push_back(instance);

    // blocking function call
    std::thread t([instance]() {
        instance->readInputFromSource();
        shutdown_requested = true;
    });
    t.detach();

    while (!shutdown_requested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    for (const auto& instance_p: instances) {
        instance_p->writeShutdownMessage();
    }
}

int parseAndExecuteCommandline(int argc,
                               char* argv[],
                               bool& found_execute_command,
                               bool& json_mode,
                               bool& auto_discovery_of_systems)
{
    static const std::vector<std::string> executeOption = {"-e", "--execute"};
    static const std::vector<std::string> autoDiscoveryOption = {"-ad", "--auto_discovery"};
    static const std::vector<std::string> jsonOption = {"--json"};
    static const std::vector<std::string> websocketModeOption = {"--websocket"};

    operation_mode = COMMANDLINE;
    for (int i = 0; i < argc; i++) {
        std::string arg = argv[i];
        if (std::find(autoDiscoveryOption.begin(), autoDiscoveryOption.end(), arg) !=
            autoDiscoveryOption.end()) {
            auto_discovery_of_systems = true;
        }
        else if (std::find(jsonOption.begin(), jsonOption.end(), arg) != jsonOption.end()) {
            json_mode = true;
        }
        else if (std::find(executeOption.begin(), executeOption.end(), arg) !=
                 executeOption.end()) {
            FepControlCommandLine new_session(json_mode);
            return new_session.processCommandline(
                std::vector<std::string>(argv + i + 1, argv + argc));
        }
        else if (std::find(websocketModeOption.begin(), websocketModeOption.end(), arg) !=
                 websocketModeOption.end()) {
            operation_mode = WEBSOCKET;
        }
    }
    // Suppress help if we are in json mode
    if (json_mode || operation_mode == WEBSOCKET) {
        return -1;
    }
    else {
        std::cerr << "invalid commandline, use: fep_control"
                     " --auto_discovery --execute <execute_command>"
                  << "\n";
        std::cerr << "                     or:  fep_control --execute <execute_command>"
                  << "\n";
        std::cerr << "                     or:  fep_control -ad -e <execute_command>"
                  << "\n";
    }
    return -1;
}

int main(int argc, char* argv[])
{
    // application settings
    bool json_mode = false;
    bool auto_discovery_of_systems = false;

#ifdef __linux__
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
#elif _WIN32
    // FIXME: Disable processed input to let system place ^C character in input buffer
    // See https://docs.microsoft.com/en-us/windows/console/getconsolemode
    // and
    // https://github.com/yhirose/cpp-linenoise/blob/a927043cdd5bfe203560802e56a7e7ed43156ed3/linenoise.hpp#L1652

    // Register signal handler to handle CTRL-C and close events
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
#endif

    if (argc > 1) {
        bool found_execute_command = false;
        int result = parseAndExecuteCommandline(argc - 1,
                                                argv + 1,
                                                found_execute_command,
                                                json_mode,
                                                auto_discovery_of_systems); // shift by one

        // If we are in json mode and no execute command was found we will fallback to interactive
        // mode otherwise exit
        if ((found_execute_command || !json_mode) && operation_mode == COMMANDLINE) {
            return result;
        }
    }

    if (operation_mode == WEBSOCKET) {
        interactiveLoopWebsocket(json_mode);
    }
    else {
        interactiveLoopCLI(json_mode);
    }

    return 0;
}
