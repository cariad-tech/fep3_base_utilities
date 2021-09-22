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
 
#include "control_tool_client.h"
#include "binary_tool_path.h"
#include "control_tool_websocket_test.h"

#include <a_util/filesystem.h>
#include <a_util/strings.h>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/process.hpp>
#include <chrono>
#include <fep_system/fep_system.h>
#include <gtest/gtest.h>
#include <json/json.h>
#include <thread>
#include <unordered_set>

#ifdef _WIN32
#include "sysinfoapi.h"
#endif
namespace bp = boost::process;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using namespace std::chrono_literals;

/**
Test clean shutdown if a signal is sent to the application.
*/
TEST_F(ControlToolWebsocket, testCleanShutdown)
{
    // cannot use boost process here because process must be independent to kill it properly
    // start an independent instance of fep_control
#ifdef __linux__
    std::string cmd = binary_tool_path + " --websocket &";
#elif _WIN32
    std::string cmd = "start " + binary_tool_path + " --websocket";
#endif

    // system call won't work with special characters
    cmd.erase(std::remove(cmd.begin(), cmd.end(), '\"'), cmd.end());

    system(cmd.c_str());

    ControlToolClient client;
    bool is_connected = client.connectWebsocket();
    ASSERT_EQ(is_connected, true);

    std::thread t([]() {
        // we must wait here for a while to listen for incomming messages before we kill the
        // application otherwise, EOF flag will be read instead of bye message
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
#ifdef __linux__
        // kill(getProcIdByName("fep_control"), SIGTERM);
        system("pkill -TERM fep_control");
#elif _WIN32
        char system32[MAX_PATH];
        GetSystemDirectoryA(system32, sizeof(system32));
        std::string path = std::string(system32) + "\\taskkill /IM fep_control.exe";
        system(path.c_str());
#endif
    });

    try {
        // Wait until shutdown message is received
        beast::flat_buffer buffer;
        std::string answer = client.receiveMessage();
        std::string expected_prefix = "bye";
        ASSERT_EQ(answer.compare(0u, expected_prefix.size(), expected_prefix), 0);
    }
    catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        // return EXIT_FAILURE;
    }

    ASSERT_EQ(client.closeWebsocket(), true);
    // special case: we must wait here because we won't receive a message from application under
    // test
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    t.join();
}

TEST_F(ControlToolWebsocket, testGetCurrentWorkingDirectory)
{
    bp::child c(binary_tool_path + " --websocket");

    ControlToolClient client;

    bool is_connected = client.connectWebsocket();
    ASSERT_EQ(is_connected, true);

    try {
        client.sendMessage("getCurrentWorkingDirectory");
        std::string answer = client.receiveMessage();
        // Test the result
        ASSERT_TRUE(c.running());
        a_util::strings::trim(answer);
        std::string expected_prefix = "working directory : ";
        ASSERT_EQ(answer.compare(0u, expected_prefix.size(), expected_prefix), 0);
        a_util::filesystem::Path current_dir(answer.substr(expected_prefix.size()));
        EXPECT_TRUE(current_dir.isAbsolute());
    }
    catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    ASSERT_EQ(client.closeWebsocket(), true);
    // Receiving close frame
}

TEST_F(ControlToolWebsocket, testSystemHandling)
{
    // Start FEP Control in websocket mode
    bp::child c(binary_tool_path + " --websocket");

    ControlToolClient client;

    // give application time to establish websocket before connecting
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    ASSERT_TRUE(testSystemHandling(client));
}

TEST_F(ControlToolWebsocket, testSystemHandling_multiClient)
{
    bp::child c(binary_tool_path + " --websocket");

    ControlToolClient client1;
    ControlToolClient client2;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    ASSERT_TRUE(testSystemHandling(client1));
    ASSERT_TRUE(testSystemHandling(client2));
}
