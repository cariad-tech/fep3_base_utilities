/**
 * @copyright
 * @verbatim
 * Copyright @ 2021 VW Group. All rights reserved.
 *
 *     This Source Code Form is subject to the terms of the Mozilla
 *     Public License, v. 2.0. If a copy of the MPL was not distributed
 *     with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * If it is not possible or desirable to put the notice in a particular file, then
 * You may include the notice in a location (such as a LICENSE file in a
 * relevant directory) where a recipient would be likely to look for such a notice.
 *
 * You may add additional accurate notices of copyright ownership.
 *
 * @endverbatim
 */

#include "control_tool_websocket_test.h"
#include "control_tool_client.h"

#include "../../../../../src/fep_control_tool/control_tool_common_helper.h"

#include <a_util/filesystem.h>

testing::AssertionResult ControlToolWebsocket::testSystemHandling(ControlToolClient& client)
{
    const auto test_files_path = a_util::filesystem::getWorkingDirectory() + "files";
    std::string set_working_directory_command =
        "setCurrentWorkingDirectory " + quoteNameIfNecessary(test_files_path.toString());

    client.connectWebsocket();
    try {
        TestParticipants test_parts;
        createSystem(test_parts, false);

        client.sendMessage(set_working_directory_command);
        std::string answer = client.receiveMessage();

        // connect system
        client.sendMessage("discoverSystem " + _system_name);
        const std::vector<std::string> expected_answer_participants = {
            _system_name, ":", "test_part_0,", "test_part_1"};
        answer = client.receiveMessage();

        for (const auto str: expected_answer_participants) {
            std::size_t found = answer.find(str);
            if (found == std::string::npos) {
                return testing::AssertionFailure()
                       << __FILE__ << __LINE__ << "discoverSystem failed.";
            }
        }

        // start system
        client.sendMessage("startSystem " + _system_name);
        const std::vector<std::string> expected_answer_started = {_system_name, "started"};
        answer = client.receiveMessage();

        for (const auto str: expected_answer_started) {
            std::size_t found = answer.find(str);
            if (found == std::string::npos) {
                return testing::AssertionFailure() << __FILE__ << "(" << __LINE__ << ")"
                                                   << " startSystem failed.";
            }
        }

        // to make sure that the system is started at this point,
        // we sleep for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // start monitoring system
        client.sendMessage("startMonitoringSystem " + _system_name);
        const std::vector<std::string> expected_answer_monitoring = {"monitoring", "enabled"};
        answer = client.receiveMessage();

        for (const auto str: expected_answer_monitoring) {
            std::size_t found = answer.find(str);
            if (found == std::string::npos) {
                return testing::AssertionFailure()
                       << __FILE__ << __LINE__ << "startMonitoringSystem failed.";
            }
        }

        // stop system
        // we expect 2 messages here, but cannot guaratnee their order,
        // so we concatenate them
        client.sendMessage("stopSystem " + _system_name);
        const std::vector<std::string> expected_answer_stopped = {
            _system_name, "stopped", "LOG", "successfully"};
        answer = client.receiveMessage();
        answer += client.receiveMessage();

        for (const auto str: expected_answer_stopped) {
            std::size_t found = answer.find(str);
            if (found == std::string::npos) {
                return testing::AssertionFailure() << __FILE__ << __LINE__ << "stopSystem failed.";
            }
        }
    }
    catch (std::exception const& e) {
        return testing::AssertionFailure() << __FILE__ << __LINE__ << e.what();
    }

    if (!client.closeWebsocket()) {
        return testing::AssertionFailure() << __FILE__ << __LINE__ << "Websocket not closed";
    }

    return testing::AssertionSuccess();
}
