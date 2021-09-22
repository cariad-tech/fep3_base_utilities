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

#ifndef CONTROL_TOOL_TEST_SYSTEM
#define CONTROL_TOOL_TEST_SYSTEM

#include "fep_system/participant_proxy.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include <gtest/gtest.h>

namespace fep3
{
    class System;
}

class ControlToolTestSystem {
public:
    ControlToolTestSystem(std::vector<std::string> participant_names,
                          const std::string& host_name);
    testing::AssertionResult loadAndInitialiaze();
    testing::AssertionResult start();
    const std::string getSystemName() const;
private:
    testing::AssertionResult createSystemWithDiscoverTryLimit();
    testing::AssertionResult setParticipantLogging();
    void load();
    void initialize();
    bool checkParticipantsUnloaded();
    testing::AssertionResult waitForSystemState(fep3::rpc::ParticipantState expectedState);
    bool isSystemInState(fep3::rpc::ParticipantState expectedState);
    testing::AssertionResult callFunctionWithTries(std::function<bool()> tryFunction);

    const std::vector<std::string> _participant_names;
    const std::string _system_name;
    std::unique_ptr<fep3::System> _system;
    const std::chrono::milliseconds _wait_time_between_tries{200};
    const uint8_t _wait_tries_limit = 20;
};

#endif //CONTROL_TOOL_TEST_SYSTEM
