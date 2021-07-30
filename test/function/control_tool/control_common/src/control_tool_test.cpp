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

#include "control_tool_test.h"

#include "control_tool_client.h"
#include "control_tool_test_system.h"
#include "fep_assert.h"

#include <a_util/filesystem.h>
#include <a_util/strings.h>
#include <boost/asio/ip/host_name.hpp>

std::string ControlTool::getStreamUntilPromt(bp::child& c, bp::ipstream& reader_stream)
{
    std::string return_string;
    std::string str;
    for (;;) {
        reader_stream >> str;
        if (str == "fep>") {
            break;
        }
        return_string.append(str);
    }
    return return_string;
}

void ControlTool::skipUntilPrompt(bp::child& c, bp::ipstream& reader_stream)
{
    std::string str;
    for (;;) {
        ASSERT_TRUE(c.running());
        reader_stream >> str;
        if (str == "fep>") {
            break;
        }
    }
}

testing::AssertionResult ControlTool::checkUntilPrompt(
    bp::child& c, bp::ipstream& reader_stream, const std::vector<std::string>& expected_answer)
{
    std::string skippables;
    const char* stop_search = "fep>";
    std::string str;
    std::string actual_answer;

    size_t i = 0u;

    for (;; ++i) {
        if (!c.running()) {
            return testing::AssertionFailure() << " fep_control not running";
        }

        reader_stream >> str;
        if (str == stop_search || i >= expected_answer.size()) {
            break;
        }

        if (str == expected_answer[i]) {
            actual_answer += str + " ";
            continue;
        }
        else {
            skippables += str + " ";
            i--;
        }
    }

    a_util::strings::trim(skippables);
    a_util::strings::trim(actual_answer);

    if (actual_answer.empty()) {
        actual_answer = skippables;
    }

    if (i != expected_answer.size()) {
        return testing::AssertionFailure()
               << "Words expected: " << a_util::strings::join(expected_answer, " ")
               << "\n Words received: " << actual_answer;
    }

    if (!actual_answer.empty() && skippables.size() > 0) {
        std::cout << "Skipped words on std out: " << skippables << std::endl;
    }

    return testing::AssertionSuccess();
}

std::vector<Json::Value> ControlTool::readJsonArray(const std::string& in_json_string)
{
    std::vector<Json::Value> parsed_values;
    std::string json_string(in_json_string);
    a_util::strings::trim(json_string);

    std::string delim = "}";
    Json::Value root;
    auto start = 0U;
    auto end = json_string.find(delim);

    Json::CharReaderBuilder readerBuilder;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());

    while (end != std::string::npos) {
        std::string substring = json_string.substr(start, end - start + 1);

        start = end + delim.length();
        end = json_string.find(delim, start);

        std::string error;
        Json::Value root;
        std::istringstream inputstream(json_string);

        reader->parse(&substring.front(), &substring.back(), &root, &error);
        parsed_values.push_back(std::move(root));
    }

    return parsed_values;
}

Json::Value ControlTool::readJsonArray(bp::ipstream& reader_stream)
{
    std::string json_string;

    std::getline(reader_stream, json_string);
    a_util::strings::trim(json_string);

    std::string error;
    Json::Value root;
    static Json::CharReaderBuilder rbuilder;
    std::istringstream inputstream(json_string);
    Json::parseFromStream(rbuilder, inputstream, &root, &error);
    return root;
}
void ControlTool::closeSession(bp::child& c, bp::opstream& writer_stream)
{
    ASSERT_TRUE(c.running()) << __FILE__ << __LINE__ << " fep_control not running";
    writer_stream << std::endl << "quit" << std::endl;
    c.wait();
}

ControlTool::TestParticipants ControlTool::createTestParticipants(
    const std::vector<std::string>& participant_names, const std::string& system_name)
{
    using namespace fep3::core;
    TestParticipants test_parts;
    std::for_each(participant_names.begin(), participant_names.end(), [&](const std::string& name) {
        auto part = createParticipant<ElementFactory<TestElement>>(name, "1.0", system_name);
        auto part_exec = std::make_unique<PartStruct>(std::move(part));
        part_exec->_part_executor.exec();
        test_parts[name].reset(part_exec.release());
    });
    return std::move(test_parts);
}

testing::AssertionResult ControlTool::createSystem(TestParticipants& test_parts, bool start_system)
{
    const std::string sys_name = "FEP_SYSTEM"; // +  boost::asio::ip::host_name();
    const std::vector<std::string> participant_names = {"test_part_0", "test_part_1"};
    ControlToolTestSystem testSystem{participant_names, "FEP_SYSTEM"};
    _system_name = testSystem.getSystemName();
    test_parts = createTestParticipants(participant_names, _system_name);

    FEP_RETURN_ON_FAIL(testSystem.loadAndInitialiaze());
    if (start_system) {
        FEP_RETURN_ON_FAIL(testSystem.start());
    }
    return testing::AssertionSuccess();
}
