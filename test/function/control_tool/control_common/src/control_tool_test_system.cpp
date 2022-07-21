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

#include "control_tool_test_system.h"

#include <algorithm>
#include <boost/asio/ip/host_name.hpp>
#include <fep_system/fep_system.h>
#include <thread>

#include "fep_assert.h"

namespace {
std::string toString(const fep3::System::AggregatedState st)
{
    switch (st) {
    case fep3::System::AggregatedState::undefined:
        return "undefined";
    case fep3::System::AggregatedState::unreachable:
        return "unreachable";
    case fep3::System::AggregatedState::unloaded:
        return "unloaded";
    case fep3::System::AggregatedState::loaded:
        return "loaded";
    case fep3::System::AggregatedState::initialized:
        return "initialized";
    case fep3::System::AggregatedState::paused:
        return "paused";
    case fep3::System::AggregatedState::running:
        return "running";
    default:
        break;
    }
    return "NOT RESOLVABLE";
}

fep3::rpc::arya::ParticipantState getParticipantState(const fep3::ParticipantProxy& part)
{
    auto state_machine = part.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>();
    if (state_machine) {
        auto value = state_machine->getState();
        return value;
    }
    else {
        return fep3::rpc::arya::undefined;
    }
}
} // namespace

ControlToolTestSystem::ControlToolTestSystem(std::vector<std::string> participant_names,
                                             const std::string& host_name)
    : _participant_names(std::move(participant_names)),
      _system_name(host_name + "_" + boost::asio::ip::host_name())
{
}

const std::string ControlToolTestSystem::getSystemName() const
{
    return _system_name;
}

testing::AssertionResult ControlToolTestSystem::loadAndInitialiaze()
{
    FEP_RETURN_ON_FAIL(createSystemWithDiscoverTryLimit());
    FEP_RETURN_ON_FAIL(setParticipantLogging());
    FEP_ASSERT_NO_THROW(load());
    FEP_ASSERT_NO_THROW(initialize());
    return testing::AssertionSuccess();
}

testing::AssertionResult ControlToolTestSystem::start()
{
    // in case loadAndInitialiaze was not called
    assert(_system);
    FEP_ASSERT_NO_THROW(_system->start());
    return testing::AssertionSuccess();
}

testing::AssertionResult ControlToolTestSystem::callFunctionWithTries(std::function<bool()> tryFunction)
{
    for (uint8_t try_count = 0; try_count < _wait_tries_limit; ++try_count) {
        if (tryFunction()) {
            return testing::AssertionSuccess();
        }
        else {
            std::this_thread::sleep_for(_wait_time_between_tries);
        }
    }
    return testing::AssertionFailure() << __FILE__ << __LINE__ << " callFunctionWithTries failed";
}

testing::AssertionResult ControlToolTestSystem::createSystemWithDiscoverTryLimit()
{   
    if (!callFunctionWithTries([&]() { return checkParticipantsUnloaded(); })) {
        return testing::AssertionFailure() << __FILE__ << __LINE__ << " try count reached, system creation failed";
    }
    else {
        return testing::AssertionSuccess();
    }
}

bool ControlToolTestSystem::checkParticipantsUnloaded()
{
    std::vector<bool> participants_unloaded;
    _system = std::make_unique<fep3::System>(fep3::discoverSystem(_system_name));

    for (const std::string& participant_name: _participant_names) {
        try {
            auto participant = _system->getParticipant(participant_name);
            if (participant) {
                auto value = getParticipantState(participant);
                participants_unloaded.push_back(value == fep3::rpc::arya::unloaded);
            }
            else {
                participants_unloaded.push_back(false);
            }
        }
        catch (const std::exception&) {
            // means participant is not yet there
            participants_unloaded.push_back(false);
        }
    }

    return std::all_of(participants_unloaded.begin(),
                       participants_unloaded.end(),
                       [](bool isUnloaded) { return isUnloaded; });
}

testing::AssertionResult ControlToolTestSystem::setParticipantLogging()
{
    for (const std::string& participant_name: _participant_names) {
        // this should not throw, we have checked in checkParticipantsUnloaded()
        auto participant = _system->getParticipant(participant_name);

        auto logging_service =
            participant.getRPCComponentProxyByIID<fep3::rpc::IRPCLoggingService>();
        if (logging_service) {
            logging_service->setLoggerFilter("participant",
                                             {fep3::LoggerSeverity::off, {"console"}});
        }
        else {
            return testing::AssertionFailure() << __FILE__ << __LINE__ << "participant logger not set";
        }
    }

    return testing::AssertionSuccess();
}

void ControlToolTestSystem::load()
{
    _system->load();
}

void ControlToolTestSystem::initialize()
{
    _system->initialize();
}

testing::AssertionResult ControlToolTestSystem::waitForSystemState(fep3::rpc::ParticipantState expectedState)
{
    bool result = callFunctionWithTries([&]() { return isSystemInState(expectedState);});
    auto participant_proxies = _system->getParticipants();

    if (!result)
    {
         return testing::AssertionFailure() << __FILE__ << __LINE__ << "try count reached, system state "
         << toString(expectedState) << " not reached, failed\n";
    }
    else {
        return testing::AssertionSuccess();
    }
}

bool ControlToolTestSystem::isSystemInState(fep3::rpc::ParticipantState expectedState)
{
    fep3::System::State current_system_state = _system->getSystemState()._state;

    return (current_system_state._homogeneous && 
            current_system_state._state == expectedState);
}