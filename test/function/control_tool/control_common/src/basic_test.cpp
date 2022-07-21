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
#include "../../../../../src/fep_control_tool/control_tool_common_helper.h"
#include "binary_tool_path.h"
#include "control_tool_test.h"

#include <a_util/filesystem.h>
#include <a_util/strings.h>
#include <chrono>
#include <fep3/components/clock/clock_service_intf.h>
#include <thread>
#include <unordered_set>

/**
 * Test display of welcome message after start
 *
 * @req_id          ???
 * @testData          none
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testWelcomeMessage)
{
    bp::opstream writer_stream;
    bp::ipstream reader_stream;

    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    const std::vector<std::string> expected_answer = {
        "******************************************************************",
        "*",
        "Welcome",
        "to",
        "FEP",
        "Control(c)",
        "2021",
        "VW",
        "Group",
        "*",
        "*",
        "use",
        "help",
        "to",
        "print",
        "help",
        "*",
        "******************************************************************"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer));
    closeSession(c, writer_stream);
}

/**
 * Test output of help message at command 'help'
 *
 * @req_id          ???
 * @testData        none
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testHelp)
{
    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "help" << std::endl;

    std::vector<std::string> commands = {
        "exit",
        "quit",
        "discoverAllSystems",
        "discoverSystem",
        "setCurrentWorkingDirectory",
        "getCurrentWorkingDirectory",
        "help",
        "loadSystem",
        "unloadSystem",
        "setInitPriority",
        "getInitPriority",
        "setStartPriority",
        "getStartPriority",
        "initializeSystem",
        "deinitializeSystem",
        "startSystem",
        "stopSystem",
        "pauseSystem",
        "shutdownSystem",
        "startMonitoringSystem",
        "stopMonitoringSystem",
        "loadParticipant",
        "unloadParticipant",
        "initializeParticipant",
        "deinitializeParticipant",
        "startParticipant",
        "stopParticipant",
        "pauseParticipant",
        "getParticipantPropertyNames",
        "getParticipantProperties",
        "getParticipantProperty",
        "setParticipantProperty",
        "getParticipantRPCObjects",
        "getParticipantRPCObjectIIDs",
        "getParticipantRPCObjectIIDDefinition",
        "shutdownParticipant",
        "getSystemState",
        "setSystemState",
        "getParticipantState",
        "setParticipantState",
        "getParticipants",
        "callRPC",
        "configureTiming3SystemTime",
        "configureTiming3DiscreteTime",
        "configureTiming3NoSync",
        "getCurrentTimingMaster",
        "enableAutoDiscovery",
        "disableAutoDiscovery",
    };
    std::vector<std::string> listed_commands;

    std::string str, laststr;
    for (;;) {
        reader_stream >> str;
        if (str == "fep>") {
            break;
        }
        if (str == ":") {
            listed_commands.emplace_back(std::move(laststr));
        }
        laststr = std::move(str);
    }
    ASSERT_EQ(commands.size(), listed_commands.size());

    std::sort(commands.begin(), commands.end());
    std::sort(listed_commands.begin(), listed_commands.end());

    EXPECT_EQ(commands, listed_commands);

    writer_stream << "help help" << std::endl;
    const std::vector<std::string> expected_answer = {"help",
                                                      "<command",
                                                      "name>",
                                                      ":",
                                                      "prints",
                                                      "out",
                                                      "the",
                                                      "description",
                                                      "of",
                                                      "the",
                                                      "commands"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer));
    closeSession(c, writer_stream);
}

/**
 * Test reading the current work directory (absolute path)
 *
 * @req_id          ???
 * @testData        none
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testGetCurrentWorkingDirectory)
{
    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    std::string line;
    writer_stream << "getCurrentWorkingDirectory" << std::endl;
    ASSERT_TRUE(c.running());
    ASSERT_TRUE(std::getline(reader_stream, line));
    a_util::strings::trim(line);
    std::string expected_prefix = "working_directory : ";
    ASSERT_EQ(line.compare(0u, expected_prefix.size(), expected_prefix), 0);
    a_util::filesystem::Path current_dir(line.substr(expected_prefix.size()));
    EXPECT_TRUE(current_dir.isAbsolute());
    closeSession(c, writer_stream);
}

/**
 * Test setting a new current work directory
 *
 * @req_id          ???
 * @testData        none
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testSetCurrentWorkingDirectory)
{
    const auto current_path = a_util::filesystem::getWorkingDirectory();

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    std::string line;
    const auto new_path = current_path + "files";
    writer_stream << "setCurrentWorkingDirectory " << quoteNameIfNecessary(new_path.toString())
                  << std::endl;
    ASSERT_TRUE(c.running());
    ASSERT_TRUE(std::getline(reader_stream, line));
    a_util::strings::trim(line);
    std::string expected_prefix = "working_directory : ";
    ASSERT_EQ(line.compare(0u, expected_prefix.size(), expected_prefix), 0);
    a_util::filesystem::Path returned_new_path(line.substr(expected_prefix.size()));
    ASSERT_TRUE(returned_new_path.isAbsolute());
    EXPECT_EQ(returned_new_path, new_path);
    closeSession(c, writer_stream);
    a_util::filesystem::setWorkingDirectory(current_path);
}

/**
 * Test discover all systems. The test should
 * discover the test system with 2 participants
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testDiscoverAllSystems)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverAllSystems" << std::endl;
    std::string ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find(_system_name + ":test_part_0,test_part_1") != std::string::npos) <<
    " discoverAllSystems did not return :test_part_0,test_part_1 instead returned:" << ss;

    closeSession(c, writer_stream);
}

/**
 * Test of system handling. Test system
 * will be switched to all possible states
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testSystemHandling)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts, false));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverSystem " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_participants = {
        _system_name, ":", "test_part_0,", "test_part_1"};

    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_participants));

    writer_stream << "getParticipants " << _system_name << std::endl;
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_participants));

    writer_stream << "getSystemState " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_state_initialized = {
        "4", ":", "initialized", ":", "homogeneous"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_state_initialized));

    writer_stream << "startSystem " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_started = {_system_name, "started"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_started));

    writer_stream << "getSystemState " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_state_running = {
        "6", ":", "running", ":", "homogeneous"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_state_running));

    writer_stream << "stopSystem " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_stopped = {_system_name, "stopped"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_stopped));

    writer_stream << "getSystemState " << _system_name << std::endl;
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_state_initialized));

    writer_stream << "deinitializeSystem " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_deinitialized = {_system_name, "deinitialized"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_deinitialized));

    writer_stream << "getSystemState " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_state_loaded = {
        "3", ":", "loaded", ":", "homogeneous"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_state_loaded));

    writer_stream << "unloadSystem " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_unloaded = {_system_name, "unloaded"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_unloaded));

    writer_stream << "getSystemState " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_state_unloaded = {
        "2", ":", "unloaded", ":", "homogeneous"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_state_unloaded));

    writer_stream << "loadSystem " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_loaded = {_system_name, "loaded"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_loaded));

    writer_stream << "getSystemState " << _system_name << std::endl;
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_state_loaded));

    writer_stream << "initializeSystem " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_initialized = {_system_name, "initialized"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_initialized));

    writer_stream << "getSystemState " << _system_name << std::endl;
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_state_initialized));

    writer_stream << "shutdownSystem " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_shutdown_denied = {"cannot",
                                                                      "shutdown",
                                                                      "system",
                                                                      "'" + _system_name + "',",
                                                                      "exception:",
                                                                      "state",
                                                                      "machine",
                                                                      "shutdown",
                                                                      "denied",
                                                                      "state",
                                                                      "machine",
                                                                      "shutdown",
                                                                      "denied"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_shutdown_denied));

    writer_stream << "deinitializeSystem " << _system_name << std::endl;
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_deinitialized));

    writer_stream << "unloadSystem " << _system_name << std::endl;
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_unloaded));

    writer_stream << "shutdownSystem " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_shutdown_ok = {_system_name, "shutdowned"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_shutdown_ok));

    writer_stream << "getSystemState " << _system_name << std::endl;
    const std::vector<std::string> expected_not_connected = {
        "System", "'" + _system_name + "'", "is", "not", "connected"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_not_connected));

    closeSession(c, writer_stream);
}

/**
 * Test to discover an existing fep system
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testDiscoverSystem)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverSystem " << _system_name << std::endl;

    const std::vector<std::string> expected_answer = {
        _system_name, ":", "test_part_0,", "test_part_1"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer));
    closeSession(c, writer_stream);
}

/**
 * Test reaction to an invalid command
 *
 * @req_id          ???
 * @testData        none
 * @testType        negative test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method replys with expected hint
 */
TEST_F(ControlTool, testInvalidCommand)
{
    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "hlep" << std::endl;

    const std::vector<std::string> expected_answer = {
        "Invalid", "command", "'hlep',", "use", "'help'", "for", "valid", "commands"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer));
    closeSession(c, writer_stream);
}

/**
 * Test reaction to an invalid number of arguments
 *
 * @req_id          ???
 * @testData        none
 * @testType        negative test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method replys with expected hint
 */
TEST_F(ControlTool, testWrongNumberOfArguments)
{
    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "getCurrentWorkingDirectory c:" << std::endl;

    const std::vector<std::string> expected_answer = {"Invalid",
                                                      "number",
                                                      "of",
                                                      "arguments",
                                                      "for",
                                                      "'getCurrentWorkingDirectory'",
                                                      "(1",
                                                      "instead",
                                                      "of",
                                                      "0),",
                                                      "use",
                                                      "'help'",
                                                      "for",
                                                      "more",
                                                      "information"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer));
    closeSession(c, writer_stream);
}

/**
 * Test usage of command line arguments
 *
 * @req_id          ???
 * @testData        none
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testCommandlineArgument)
{
    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path + " -e getCurrentWorkingDirectory",
                bp::std_out > reader_stream,
                bp::std_in < writer_stream);

    ASSERT_TRUE(c.running());

    std::string line;
    ASSERT_TRUE(std::getline(reader_stream, line));
    a_util::strings::trim(line);
    std::string expected_prefix = "working_directory : ";
    ASSERT_EQ(line.compare(0u, expected_prefix.size(), expected_prefix), 0);
    a_util::filesystem::Path current_dir(line.substr(expected_prefix.size()));
    EXPECT_TRUE(current_dir.isAbsolute());
    // Session should already be closed
    // closeSession(c, writer_stream);
}

/**
 * Test exit of test object
 *
 * @req_id          ???
 * @testData        none
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  test object is no longer running
 */
TEST_F(ControlTool, testExit)
{
    using namespace std::chrono_literals;

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "exit" << std::endl;
    std::this_thread::sleep_for(1s);

    EXPECT_FALSE(c.running());
}

/**
 * Test quit of test object
 *
 * @req_id          ???
 * @testData        none
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  test object is no longer running
 */

TEST_F(ControlTool, testQuit)
{
    using namespace std::chrono_literals;

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "quit" << std::endl;
    std::this_thread::sleep_for(1s);

    EXPECT_FALSE(c.running());
}

/**
 * Test of participant handling. Participants of the
 * test system will be switched to all possible states
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testParticipantStates)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts, false));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverAllSystems" << std::endl;
    std::string ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find(_system_name + ":test_part_0,test_part_1") != std::string::npos);

    using State = fep3::SystemAggregatedState;
    const std::vector<std::string> states_desc = {"undefined",   // 0
                                                  "unreachable", // 1
                                                  "unloaded",    // 2
                                                  "loaded",      // 3
                                                  "initialized", // 4
                                                  "paused",      // 5
                                                  "running"};

    auto check_states =
        [&](State system_state, int homogenous, State part0_state, State part1_state) {
            writer_stream << "getSystemState " << _system_name << std::endl;
            const std::vector<std::string> expected_answer_system_state = {
                std::to_string(system_state),
                ":",
                states_desc[system_state],
                ":",
                homogenous ? "homogeneous" : "inhomogeneous"};

            EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_system_state));

            writer_stream << "getParticipantState " << _system_name << " test_part_0" << std::endl;
            const std::vector<std::string> expected_answer_part0_state = {
                std::to_string(part0_state), ":", states_desc[part0_state]};
            EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_part0_state));

            writer_stream << "getParticipantState " << _system_name << " test_part_1" << std::endl;
            const std::vector<std::string> expected_answer_part1_state = {
                std::to_string(part1_state), ":", states_desc[part1_state]};
            EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_part1_state));
        };

    auto action_participant = [&](const std::string& participant_number,
                                  const std::string& action,
                                  const std::string& result) {
        writer_stream << action << "Participant " << _system_name << " test_part_"
                      << participant_number << std::endl;
        const std::vector<std::string> expected_answer = {
            "test_part_" + participant_number + "@" + _system_name, result};
        EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer));
    };

    check_states(State::initialized, 1, State::initialized, State::initialized);
    action_participant("0", "start", "started");
    check_states(State::initialized, 0, State::running, State::initialized);
    action_participant("1", "start", "started");
    check_states(State::running, 1, State::running, State::running);
    action_participant("0", "stop", "stopped");
    check_states(State::initialized, 0, State::initialized, State::running);
    action_participant("1", "stop", "stopped");
    check_states(State::initialized, 1, State::initialized, State::initialized);
    action_participant("0", "deinitialize", "deinitialized");
    check_states(State::loaded, 0, State::loaded, State::initialized);
    action_participant("0", "unload", "unloaded");
    check_states(State::unloaded, 0, State::unloaded, State::initialized);
    action_participant("0", "load", "loaded");
    check_states(State::loaded, 0, State::loaded, State::initialized);
    action_participant("0", "initialize", "initialized");
    check_states(State::initialized, 1, State::initialized, State::initialized);

    writer_stream << "setParticipantState " << _system_name << " test_part_1 running" << std::endl;
    const std::vector<std::string> expected_answer_part_state_running = {"6", ":", "running"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_part_state_running));

    check_states(State::initialized, 0, State::initialized, State::running);

    closeSession(c, writer_stream);
}

/**
 * Test property handling of a participant
 * sets and gets properties of a participant
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testProperties)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts, false));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverAllSystems" << std::endl;
    std::string ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find(_system_name + ":test_part_0,test_part_1") != std::string::npos);

    // Read a Property leaf
    writer_stream << "getParticipantProperty " << _system_name
                  << " test_part_0 clock_synchronization/timing_master" << std::endl;
    const std::vector<std::string> expected_answer_empty = {"timing_master", ":"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_empty));

    // Read a Property node
    writer_stream << "getParticipantProperty " << _system_name << " test_part_0 clock" << std::endl;
    const std::vector<std::string> expected_answer_read_node = {
        "main_clock",
        ":",
        "local_system_realtime",
        FEP3_TIME_UPDATE_TIMEOUT_PROPERTY,
        ":",
        a_util::strings::toString(FEP3_TIME_UPDATE_TIMEOUT_DEFAULT_VALUE),
        "time_factor",
        ":",
        "1.000000",
        FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY,
        ":",
        a_util::strings::toString(FEP3_CLOCK_SIM_TIME_STEP_SIZE_DEFAULT_VALUE)};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_read_node));

    // Set property
    writer_stream << "setParticipantProperty " << _system_name
                  << " test_part_0 "
                     "clock_synchronization/timing_master test_part_1"
                  << std::endl;
    const std::vector<std::string> expected_answer_set = {"property", "set"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_set));

    // Read the Property
    writer_stream << "getParticipantProperty " << _system_name
                  << " test_part_0 clock_synchronization/timing_master" << std::endl;
    const std::vector<std::string> expected_answer_get = {"timing_master", ":", "test_part_1"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_get));

    // Read all default Properties - only test for the first leaf
    writer_stream << "getParticipantProperties " << _system_name << " test_part_0" << std::endl;
    ASSERT_TRUE(c.running());

    std::string line;
    ASSERT_TRUE(std::getline(reader_stream, line));
    a_util::strings::trim(line);
    std::string expected_prefix = std::string("test_part_0 :");
    ASSERT_EQ(line.compare(0u, expected_prefix.size(), expected_prefix), 0);

    ASSERT_TRUE(std::getline(reader_stream, line));
    a_util::strings::trim(line);
    expected_prefix = "logging :";
    ASSERT_EQ(line.compare(0u, expected_prefix.size(), expected_prefix), 0);

    ASSERT_TRUE(std::getline(reader_stream, line));
    a_util::strings::trim(line);
    expected_prefix = "default_sinks : console,rpc";
    ASSERT_EQ(line.compare(0u, expected_prefix.size(), expected_prefix), 0);

    closeSession(c, writer_stream);
}

/**
 * Test property handling of a participant
 * sets and gets properties of a participant
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testProperties_json)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts, false));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    // Enable Json Mode first
    writer_stream << "enableJson" << std::endl;
    auto root = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);
    ASSERT_TRUE(root.isObject());
    EXPECT_EQ(root["action"].asString(), "enableJson");
    EXPECT_EQ(root["value"]["note"].asString(), "json_mode: enabled") << "enableJson";


    writer_stream << "discoverSystem " << _system_name << std::endl;
    auto ret = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);
    ASSERT_TRUE(ret.isObject());
    EXPECT_EQ(ret["action"].asString(), "discoverSystem");
    EXPECT_EQ(ret["value"]["system_name"].asString(), _system_name);
    EXPECT_EQ(ret["value"]["participants"].asString(), "test_part_0, test_part_1");


    // Read a Property leaf
    writer_stream << "getParticipantProperty " << _system_name
        << " test_part_0 clock" << std::endl;
    auto ret_2 = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);
    ASSERT_TRUE(ret_2.isObject());
    EXPECT_EQ(ret_2["action"].asString(), "getParticipantProperty");
    EXPECT_EQ(ret_2["value"]["participant_property"]["sub_properties"][0]["name"].asString(), "main_clock");
    EXPECT_EQ(ret_2["value"]["participant_property"]["sub_properties"][0]["value"].asString(), "local_system_realtime");
    EXPECT_EQ(ret_2["value"]["participant_property"]["sub_properties"][1]["name"].asString(), "time_update_timeout");
    EXPECT_EQ(ret_2["value"]["participant_property"]["sub_properties"][1]["value"].asString(), "5000000000");
    EXPECT_EQ(ret_2["value"]["participant_property"]["sub_properties"][2]["name"].asString(), "time_factor");
    EXPECT_EQ(ret_2["value"]["participant_property"]["sub_properties"][2]["value"].asString(), "1.000000");
    EXPECT_EQ(ret_2["value"]["participant_property"]["sub_properties"][3]["name"].asString(), "step_size");
    EXPECT_EQ(ret_2["value"]["participant_property"]["sub_properties"][3]["value"].asString(), "100000000");


    // Read the Property
    writer_stream << "getParticipantProperty " << _system_name
        << " test_part_0 clock/main_clock" << std::endl;
    auto ret_3 = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);
    ASSERT_TRUE(ret_3.isObject());
    EXPECT_EQ(ret_3["action"].asString(), "getParticipantProperty");
    EXPECT_EQ(ret_3["value"]["participant_property"]["name"].asString(), "main_clock");
    EXPECT_EQ(ret_3["value"]["participant_property"]["value"].asString(), "local_system_realtime");

    closeSession(c, writer_stream);
}


/**
 * Test getting a non existent property via 'getParticipantProperty'
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        negative test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected error message
 */
TEST_F(ControlTool, testGetPropertyInvalidPath)
{
	TestParticipants test_parts;
	ASSERT_TRUE(createSystem(test_parts, false));

	bp::opstream writer_stream;
	bp::ipstream reader_stream;
	bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
	skipUntilPrompt(c, reader_stream);

	writer_stream << "discoverAllSystems" << std::endl;
	std::string ss = getStreamUntilPromt(c, reader_stream);
	EXPECT_TRUE(ss.find(_system_name + ":test_part_0,test_part_1") != std::string::npos);

	writer_stream << "getParticipantProperty " << _system_name
		<< " test_part_0 "
		"foo"
		<< std::endl;
	const std::vector<std::string> expected_answer_set_node = { "cannot", "get", "property", "foo", "Property", "'foo'", "does", "not", "exist." };
	EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_set_node));

	closeSession(c, writer_stream);
}

/**
 * Test setting a non existent property via 'setParticipantProperty'
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        negative test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected error message
 */
TEST_F(ControlTool, testSetPropertyInvalidPath)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts, false));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverAllSystems" << std::endl;
    std::string ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find(_system_name + ":test_part_0,test_part_1") != std::string::npos);

	writer_stream << "setParticipantProperty " << _system_name
		<< " test_part_0 "
		"foo 1"
		<< std::endl;
	const std::vector<std::string> expected_answer_set_node = { "cannot", "set", "property", "foo", "Property", "'foo'", "does", "not", "exist." };
	EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_set_node));

    closeSession(c, writer_stream);
}

/**
 * Test handling of timing master of the system
 * execute get and configure functions for master
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testTimingMaster)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts, false));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverAllSystems" << std::endl;

    std::string ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find(_system_name+":test_part_0,test_part_1") != std::string::npos);

    writer_stream << "getCurrentTimingMaster " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_empty_timing_masters = {
        "timing_masters", ":"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_empty_timing_masters));

    writer_stream << "configureTiming3SystemTime " << _system_name << " test_part_0" << std::endl;
    const std::vector<std::string> expected_answer_timing = {
        "successfully", "set", "SystemTimingSystemTime"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_timing));

    writer_stream << "getCurrentTimingMaster " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_part0_timing_masters = {
        "timing_masters", ":", "test_part_0"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_part0_timing_masters));

    writer_stream << "configureTiming3NoSync " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_timing_no_sync = {
        "successfully", "set", "SystemTimeNoSync"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_timing_no_sync));

    writer_stream << "getCurrentTimingMaster " << _system_name << std::endl;
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_empty_timing_masters));

    writer_stream << "configureTiming3DiscreteTime " << _system_name << " test_part_1 1.0 0.0"
                  << std::endl;
    const std::vector<std::string> expected_answer_timing_discrete = {
        "successfully", "set", "SystemTimingDiscrete"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_timing_discrete));

    writer_stream << "getCurrentTimingMaster " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_part1_timing_masters = {
        "timing_masters", ":", "test_part_1"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_part1_timing_masters));

    closeSession(c, writer_stream);
}

/**
 * Test autodiscovery functionality if enabled
 * If on, the system should be detected automatically
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testAutoDiscovery)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts, false));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "getSystemState " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_not_connected = {
        "System", "'" + _system_name + "'", "is", "not", "connected"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_not_connected));

    writer_stream << "disableAutoDiscovery" << std::endl;

    const std::vector<std::string> expected_answer_disabled = {"auto_discovery:", "disabled"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_disabled));

    writer_stream << "getSystemState " << _system_name << std::endl;
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_not_connected));

    writer_stream << "enableAutoDiscovery" << std::endl;

    const std::vector<std::string> expected_answer_enabled = {"auto_discovery:", "enabled"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_enabled));

    writer_stream << "getSystemState " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_state_initialized = {
        "4", ":", "initialized", ":", "homogeneous"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_state_initialized));
}

/**
 * Test reading properties of participants in json mode
 * Get single/all properties of one participant of the system
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testJsonProperties)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts, false));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(
        binary_tool_path + " --json", bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverAllSystems" << std::endl;
    auto root = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);
    ASSERT_TRUE(root.isObject());
    EXPECT_EQ(root["action"].asString(), "discoverAllSystems");
    EXPECT_EQ(root["value"][0]["participants"].asString(), "test_part_0, test_part_1");
    EXPECT_EQ(root["value"][0]["system_name"].asString(), _system_name);

    ASSERT_TRUE(c.running());
    writer_stream << "getParticipantProperties " << _system_name << " test_part_0" << std::endl;

    root = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);
    ASSERT_TRUE(root.isObject());

    EXPECT_EQ(root["value"]["participant_properties"][0]["name"].asString(), "logging");
    EXPECT_EQ(root["value"]["participant_properties"][0]["sub_properties"][0]["name"].asString(),
              "default_sinks");
    EXPECT_EQ(root["value"]["participant_properties"][0]["sub_properties"][0]["type"].asString(), "string");
    EXPECT_EQ(root["value"]["participant_properties"][0]["sub_properties"][0]["value"].asString(),
              "console,rpc");

    ASSERT_TRUE(c.running());
    writer_stream << "getParticipantProperty " << _system_name << " test_part_0 clock/main_clock"
                  << std::endl;

    root = readJsonArray(reader_stream);
    ASSERT_TRUE(root.isObject());

    EXPECT_EQ(root["value"]["participant_property"]["name"].asString(), "main_clock");
    EXPECT_EQ(root["value"]["participant_property"]["type"].asString(), "string");
    EXPECT_EQ(root["value"]["participant_property"]["value"].asString(), "local_system_realtime");
}

/**
 * Test monitoring of a FEP system in json mode
 * Get JSON formatted output
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */

TEST_F(ControlTool, testJsonMonitoring_enableJSON)
{
    using namespace std::chrono_literals;
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "enableJson" << std::endl;
    auto ret = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);
    ASSERT_TRUE(ret.isObject());
    EXPECT_EQ(ret["action"].asString(), "enableJson");
    EXPECT_EQ(ret["status"].asInt(), 0);
    EXPECT_EQ(ret["value"]["note"].asString(), "json_mode: enabled");

    writer_stream << "discoverSystem " << _system_name << std::endl;

    auto root = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);
    ASSERT_TRUE(root.isObject());
    EXPECT_EQ(root["action"].asString(), "discoverSystem");
    EXPECT_EQ(root["value"]["system_name"].asString(), _system_name);
    EXPECT_EQ(root["value"]["participants"].asString(), "test_part_0, test_part_1");

    ASSERT_TRUE(c.running());
    writer_stream << "startMonitoringSystem " << _system_name << std::endl;
    auto ret_monitoring = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);
    ASSERT_TRUE(ret_monitoring.isObject());
    EXPECT_EQ(ret_monitoring["action"].asString(), "startMonitoringSystem");
    EXPECT_EQ(ret_monitoring["status"].asInt(), 0);
    EXPECT_EQ(ret_monitoring["value"]["note"].asString(), "monitoring: enabled");

    writer_stream << "stopSystem " << _system_name << std::endl;

    const int num_expected_messages = 2;
    int message_count = 0;

    for (int i = 0; i < num_expected_messages; ++i) {
        root = readJsonArray(reader_stream);
        ASSERT_TRUE(root.isObject());
        if ((root["log_type"].asString() == "message")) {
            message_count++;
        }

        if ((root["action"].asString() == "stopSystem") &&
            (root["value"]["note"].asString() == _system_name + " stopped")) {
            message_count++;
        }
    }
    EXPECT_EQ(message_count, num_expected_messages);

    ASSERT_TRUE(c.running());
    writer_stream << "stopMonitoringSystem " << _system_name << std::endl;
    skipUntilPrompt(c, reader_stream);
}

/**
 * Test json output of a note message
 *
 * @req_id          ???
 * @testData        none
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testWriteNote)
{
    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(
        binary_tool_path + " --json", bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    const auto path = a_util::filesystem::getWorkingDirectory() + "files";
    writer_stream << "setCurrentWorkingDirectory " << quoteNameIfNecessary(path.toString())
                  << std::endl;

    const auto root = readJsonArray(reader_stream);
    ASSERT_TRUE(root.isObject());

    EXPECT_EQ(root["action"].asString(), "setCurrentWorkingDirectory");
    EXPECT_EQ(root["value"]["working_directory"].asString(), path);
}

/**
 * Test json output of an error message
 *
 * @req_id          ???
 * @testData        none
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testWriteError)
{
    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(
        binary_tool_path + " --json", bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    const auto path = a_util::filesystem::getWorkingDirectory() + "file";
    writer_stream << "setCurrentWorkingDirectory " << quoteNameIfNecessary(path.toString())
                  << std::endl;

    const auto root = readJsonArray(reader_stream);
    ASSERT_TRUE(root.isObject());

    EXPECT_EQ(root["action"].asString(), "setCurrentWorkingDirectory");
    EXPECT_EQ(root["value"]["error"].asString(),
              "cannot set working directory to '" + path.toString() + "'");
    EXPECT_EQ(root["value"]["reason"].asString(), "INVALID_PATH");
}

/**
 * Test json output of an exception message
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testWriteException)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(
        binary_tool_path + " --json", bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverSystem " << _system_name << std::endl;
    auto root = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "getParticipantProperty " << _system_name << " test_part_0 Clock/MainClock"
                  << std::endl;
    root = readJsonArray(reader_stream);
    ASSERT_TRUE(root.isObject());

    EXPECT_EQ(root["action"].asString(), "getParticipantProperty");
    EXPECT_EQ(root["value"]["exception"].asString(),
              "cannot get property Clock/MainClock for participant 'test_part_0@" + _system_name +
                  "\'");
    EXPECT_EQ(root["value"]["reason"].asString(), 
              "Could not execute test_part_0->configuration->getProperties - Error: ERR_PATH_NOT_FOUND - Property 'Clock' does not exist.");
}

/**
 * Test switch output between normal and json mode
 * Output will be tested with paritcipant properties
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testEnableDisableJsonMode)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverSystem " << _system_name << std::endl;

    const std::vector<std::string> expected_answer = {
        _system_name, ":", "test_part_0,", "test_part_1"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer)) << "discoverSystem when disableJson";

    // Check default getParticipantProperties
    writer_stream << "getParticipantProperties " << _system_name << " test_part_0" << std::endl;
    ASSERT_TRUE(c.running());

    std::string line;
    ASSERT_TRUE(std::getline(reader_stream, line));
    a_util::strings::trim(line);
    std::string expected_prefix = std::string("test_part_0 :");
    ASSERT_EQ(line.compare(0u, expected_prefix.size(), expected_prefix), 0) << "getParticipantProperties when disableJson";
    skipUntilPrompt(c, reader_stream);

    writer_stream << "enableJson" << std::endl;

    auto root = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);
    ASSERT_TRUE(root.isObject());
    EXPECT_EQ(root["action"].asString(), "enableJson");
    EXPECT_EQ(root["value"]["note"].asString(), "json_mode: enabled") << "enableJson"; 

    // Check JSON discoverAllSystems 
    writer_stream << "discoverAllSystems" << std::endl;
    root = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);
    ASSERT_TRUE(root.isObject());
    EXPECT_EQ(root["action"].asString(), "discoverAllSystems");
    EXPECT_EQ(root["value"][0]["participants"].asString(), "test_part_0, test_part_1") << "discoverAllSystems when enableJson";
    EXPECT_EQ(root["value"][0]["system_name"].asString(), _system_name) << "discoverAllSystems when enableJson";

    // Check JSON getParticipantProperties
    ASSERT_TRUE(c.running());
    writer_stream << "getParticipantProperties " << _system_name << " test_part_0" << std::endl;

    root = readJsonArray(reader_stream);
    skipUntilPrompt(c, reader_stream);
    ASSERT_TRUE(root.isObject());

    EXPECT_EQ(root["value"]["participant_properties"][0]["name"].asString(), "logging");
    EXPECT_EQ(root["value"]["participant_properties"][0]["sub_properties"][0]["name"].asString(),
              "default_sinks");
    EXPECT_EQ(root["value"]["participant_properties"][0]["sub_properties"][0]["type"].asString(), "string");
    EXPECT_EQ(root["value"]["participant_properties"][0]["sub_properties"][0]["value"].asString(),
              "console,rpc");

    writer_stream << "disableJson" << std::endl;
    const std::vector<std::string> expected_answer_disableJson = {"json_mode:", "disabled"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_disableJson));

    // Check default again getParticipantProperties
    writer_stream << "getParticipantProperties " << _system_name << " test_part_0" << std::endl;
    ASSERT_TRUE(c.running());

    ASSERT_TRUE(std::getline(reader_stream, line));
    a_util::strings::trim(line);
    ASSERT_EQ(line.compare(0u, expected_prefix.size(), expected_prefix), 0);
    skipUntilPrompt(c, reader_stream);
}

/**
 * @brief Test shutdownSystem with exit
 */
/**
 * Test shutdown of a fep system
 * System will be deinitialized and stoped first
 *
 * @req_id          ???
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testShutdown)
{
    using namespace std::chrono_literals;
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts, false));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);

    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverSystem " << _system_name << std::endl;

    const std::vector<std::string> expected_answer = {
        _system_name, ":", "test_part_0,", "test_part_1"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer));

    writer_stream << "deinitializeSystem " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_deinitialized = {_system_name, "deinitialized"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_deinitialized));

    writer_stream << "unloadSystem " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_unloaded = {_system_name, "unloaded"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_unloaded));

    writer_stream << "shutdownSystem " << _system_name << std::endl;
    const std::vector<std::string> expected_answer_shutdown_ok = {_system_name, "shutdowned"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_shutdown_ok));

    writer_stream << "exit" << std::endl;

    std::string str;
    reader_stream >> str;
    EXPECT_EQ(str, "bye");
    reader_stream >> str;
    EXPECT_EQ(str, "bye");
    std::this_thread::sleep_for(1s);

    EXPECT_FALSE(c.running());
}

/**
 * @brief Test get/setInitPriority
 */
/**
 * Test getting and setting initPriority of a participant
 *
 * @req_id          FEPSDK-2969
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testInitPriority)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts, false));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverAllSystems" << std::endl;
    std::string ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find(_system_name + ":test_part_0,test_part_1") != std::string::npos);

    // write a InitPriority
    writer_stream << "setInitPriority " << _system_name << " test_part_0 2" << std::endl;
    const std::vector<std::string> expected_answer_write = {
        "priority", "for", "'test_part_0'", "set"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_write));

    // read a InitPriority
    writer_stream << "getInitPriority " << _system_name << " test_part_0" << std::endl;
    const std::vector<std::string> expected_answer_read = {"priority", ":", "2"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_read));
}

/**
 * @brief Test get/setStartPriority
 */
/**
 * Test getting and setting startPriority of a participant
 *
 * @req_id          FEPSDK-2969
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */
TEST_F(ControlTool, testStartPriority)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts, false));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;
    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverAllSystems" << std::endl;
    std::string ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find(_system_name + ":test_part_0,test_part_1") != std::string::npos);

    // write a StartPriority
    writer_stream << "setStartPriority " << _system_name << " test_part_1 3" << std::endl;
    const std::vector<std::string> expected_answer_write = {
        "priority", "for", "'test_part_1'", "set"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_write));

    // read a StartPriority
    writer_stream << "getStartPriority " << _system_name << " test_part_1" << std::endl;
    const std::vector<std::string> expected_answer_read = {"priority", ":", "3"};
    EXPECT_TRUE(checkUntilPrompt(c, reader_stream, expected_answer_read));
}

/**
 * @brief Test callRPC
 */
/**
 * Test callRPC with an example RPC call
 *
 * @req_id          FEPSDK-2961
 * @testData        FEP_SYSTEM
 * @testType        positive test
 * @precondition    none
 * @postcondition   none
 * @expectedResult  method returns expected results
 */

TEST_F(ControlTool, testRPC)
{
    TestParticipants test_parts;
    ASSERT_TRUE(createSystem(test_parts, false));

    bp::opstream writer_stream;
    bp::ipstream reader_stream;

    bp::child c(binary_tool_path, bp::std_out > reader_stream, bp::std_in < writer_stream);
    skipUntilPrompt(c, reader_stream);

    writer_stream << "discoverAllSystems" << std::endl;
    std::string ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find(_system_name + ":test_part_0,test_part_1") != std::string::npos);

    // test participant info 
    //
    // RPC Definition
    //
    // {                                           
    //    "name": "getRPCServices",                   
    //    "returns": "service1;service2"              
    // },                                              
    // {                                               
    //     "name": "getRPCServiceIIDs",                
    //     "params": {                                 
    //         "rpc_service_name": "service1"          
    //     },                                          
    //     "returns": "iid1;iid2"                      
    // },                                              
    //
    // {                                               
    //     "name": "getName",                          
    //     "returns": "participant_name"               
    // }

    const std::string object_name = "participant_info";
    const std::string intf_name = "participant_info.arya.fep3.iid";
    const std::string rpc_head = "callRPC " + _system_name + " test_part_0";
    const std::string participant_info_rpc = rpc_head + " " + object_name + " "+ intf_name;

    // succeed if RPC call without request arguments 
    writer_stream << participant_info_rpc << " getName" << std::endl;
    ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find("\"result\":\"test_part_0\"") != std::string::npos)
        << "RPC call without request parameters fails";

    // fail if RPC call with invalid RPC object name
    const std::string invalid_rpc = rpc_head + " invalid_rpc_object " + intf_name + " getName";
    writer_stream << invalid_rpc << std::endl;
    ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_FALSE(ss.find("\"result\":\"test_part_0\"") != std::string::npos)
        << "RPC call with invalid RPC object name fails";

    // with parameters
    
    // succeed if RPC call with obselete request arguments 
    std::string parameters = "{\"optional\":\"nothing\"}";
    writer_stream << participant_info_rpc << " getName " << parameters << std::endl;
    ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find("\"result\":\"test_part_0\"") != std::string::npos)
        << "RPC call with obselet request arguments fails";

    // succeed if RPC call with correct reqeust arguments 
    parameters = "{\"rpc_service_name\":\"clock_service\"}";
    const std::string participant_info_service_iids_rpc = participant_info_rpc + " getRPCServiceIIDs ";

    writer_stream << participant_info_service_iids_rpc << parameters << std::endl;
    ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find("\"result\":\"clock_service.arya.fep3.iid\"") != std::string::npos)
        << "RPC call with correct request arguments fails";

    // fail if RPC call with malformatted json string in request arguments
    parameters = "{\"rpc_service_name\":\"clock_service}";
    writer_stream << participant_info_service_iids_rpc << parameters << std::endl;
    ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find("Syntaxerror") != std::string::npos)
        << "RPC call with json malformatted json string in request arguments fails";

    // test retrieving simulation time
    //
    // RPC Definition
    //
    // {
    //      "name": "getTime",
    //      "params": {
    //          "clock_name": "name1"
    //      },
    //      "returns": "int64_time" //nanosec
    // },
    std::string get_time_rpc = rpc_head + " " + 
        "clock_service " +
        "clock_service.arya.fep3.iid " +
        "getTime " + 
        "{\"clock_name\":\"local_system_simtime\"}";


    writer_stream << get_time_rpc << std::endl;
    ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find("\"result\":\"0\"") != std::string::npos) 
        << "RPC call to retrive simulation time fails";

    // test changhing logging severity
    //
    // RPC Definition
    //
    // {
    //     "name": "setLoggerFilter",
    //     "params": {
    //         "logger_name": "name",
    //         "severity": 0,
    //         "enable_sinks": "rpc,file,console"
    //     },
    //     "returns": 0 //sink creation error
    // },
    //
    // {
    //     "name": "getLoggerFilter",
    //     "params": {
    //         "logger_name": "name"
    //     },
    //     "returns": {
    //         "severity": 0,
    //         "enable_sinks": "rpc,file,console"
    //     }
    // },
    //
    // call getLoggerFilter
    std::string get_logger_filter_rpc = rpc_head + " " +
        "logging_service " +
        "logging_service.arya.fep3.iid " +
        "getLoggerFilter " + 
        "{\"logger_name\":\"participant\"}";

    writer_stream << get_logger_filter_rpc << std::endl;
    ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find("\"severity\":0") != std::string::npos);

    // call setLoggerFilter to set severity
    std::string set_logger_filter_rpc = rpc_head + " " +
        "logging_service " +
        "logging_service.arya.fep3.iid " +
        "setLoggerFilter " +
        "{\"logger_name\":\"participant\",\"severity\":1,\"enable_sinks\":\"console\"}";

    writer_stream << set_logger_filter_rpc << std::endl;
    ss = getStreamUntilPromt(c, reader_stream);
    writer_stream << get_logger_filter_rpc << std::endl;
    ss = getStreamUntilPromt(c, reader_stream);
    EXPECT_TRUE(ss.find("\"severity\":1") != std::string::npos) 
        << "RPC call to set logger severity fails";

    closeSession(c, writer_stream);
}

TEST(ControlToolCommonHelper, quoteNameIfNecessary)
{
    EXPECT_EQ(quoteNameIfNecessary(""), "\"\"");
    EXPECT_EQ(quoteNameIfNecessary("a"), "a");
    EXPECT_EQ(quoteNameIfNecessary("nothing_Special"), "nothing_Special");
    EXPECT_EQ(quoteNameIfNecessary("having space"), "\"having space\"");

    EXPECT_EQ(quoteNameIfNecessary("\""), "\\\"");
    EXPECT_EQ(quoteNameIfNecessary("\"abc"), "\\\"abc");
    EXPECT_EQ(quoteNameIfNecessary("abc\""), "abc\\\"");
    EXPECT_EQ(quoteNameIfNecessary("x\"y"), "x\\\"y");

    EXPECT_EQ(quoteNameIfNecessary("\\"), "\\\\");
    EXPECT_EQ(quoteNameIfNecessary("\\abc"), "\\\\abc");
    EXPECT_EQ(quoteNameIfNecessary("abc\\"), "abc\\\\");
    EXPECT_EQ(quoteNameIfNecessary("x\\y"), "x\\\\y");

    EXPECT_EQ(quoteNameIfNecessary("\"x\\y\""), "\\\"x\\\\y\\\"");

    EXPECT_EQ(quoteNameIfNecessary("\"x y\\"), "\"\\\"x y\\\\\"");

    EXPECT_EQ(quoteNameIfNecessary("C:\\Program Files (x86)\\"), "\"C:\\\\Program Files (x86)\\\\\"");
}
