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

#ifndef CONTROL_TOOL_TEST
#define CONTROL_TOOL_TEST

#include "test_element.h"

#include <boost/process.hpp>

#include <fep3/core.h>
#include <fep_system/fep_system.h>
#include <gtest/gtest.h>
#include <json/json.h>

namespace bp = boost::process;

class ControlToolClient;

class ControlTool : public ::testing::Test {

protected:
    void skipUntilPrompt(bp::child& c, bp::ipstream& reader_stream);
    
    testing::AssertionResult checkUntilPrompt(bp::child& c,
                                                 bp::ipstream& reader_stream,
                                                 const std::vector<std::string>& expected_answer);
    std::string getStreamUntilPromt(bp::child& c, bp::ipstream& reader_stream);
    Json::Value readJsonArray(bp::ipstream& reader_stream);
    std::vector<Json::Value> readJsonArray(const std::string& json_string);

    void closeSession(bp::child& c, bp::opstream& writer_stream);

    using TestParticipants = std::map<std::string, std::unique_ptr<PartStruct>>;

    TestParticipants createTestParticipants(const std::vector<std::string>& participant_names,
                                                    const std::string& system_name);

    testing::AssertionResult  createSystem(ControlTool::TestParticipants& test_parts,
                                                  bool start_system = true);
    std::string _system_name;
    
};
#endif // !TEST_HELPER_CPP
