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

#include "control_tool_common_helper.h"
#include "helper.h"

#include <a_util/filesystem.h>
#include <a_util/strings.h>
#include <sstream>

FepControl::FepControl(bool json_mode) : _json_mode(json_mode), monitor(*this, json_mode)
{
}

void FepControl::discoverSystemByName(const std::string& name)
{
    auto system_name = name;
    // this updates for completion
    _last_system_name_used = system_name;
    if (system_name == _empty_system_name) {
        system_name = "";
    }
    auto system = fep3::discoverSystem(system_name);
    _connected_or_discovered_systems[system.getSystemName()] = std::move(system);
}

std::map<std::string, fep3::System>::iterator FepControl::getConnectedOrDiscoveredSystem(
    const std::string& name, const bool auto_discovery)
{
    auto it = _connected_or_discovered_systems.find(name);
    if (it != _connected_or_discovered_systems.end()) {
        _last_system_name_used = name;
        return it;
    }
    else if (auto_discovery) {
        discoverSystemByName(name);
        return getConnectedOrDiscoveredSystem(name, false);
    }
    writeOutput("system \"", name, "\" is not connected\n");
    return _connected_or_discovered_systems.end();
}

std::vector<std::string> FepControl::usedPropertiesCompletion(const std::string& word_prefix)
{
    std::vector<std::string> completions;
    for (const auto& prop: _used_properties) {
        if (prop.compare(0u, word_prefix.size(), word_prefix) == 0) {
            completions.push_back(prop);
        }
    }
    return completions;
}

std::vector<std::string> FepControl::noCompletion(const std::string&)
{
    return std::vector<std::string>();
}

std::vector<std::string> FepControl::localFilesCompletion(const std::string& word_prefix)
{
    std::vector<a_util::filesystem::Path> file_list;
    a_util::filesystem::enumDirectory(".", file_list, a_util::filesystem::ED_FILES);

    std::vector<std::string> completions;
    for (const auto& file_path: file_list) {
        const std::string file_name = file_path.getLastElement().toString();
        if (file_name.compare(0u, word_prefix.size(), word_prefix) == 0) {
            completions.push_back(quoteFilenameIfNecessary(file_name));
        }
    }
    return completions;
}

std::vector<std::string> FepControl::connectedSystemsCompletion(const std::string& word_prefix)
{
    std::vector<std::string> completions;
    for (const auto& system: _connected_or_discovered_systems) {
        if (system.first.compare(0u, word_prefix.size(), word_prefix) == 0) {
            completions.push_back(system.first);
        }
    }
    return completions;
}

std::vector<std::string> FepControl::connectedParticipantsCompletion(const std::string& word_prefix)
{
    std::vector<std::string> completions;
    const auto& found_system = _connected_or_discovered_systems.find(_last_system_name_used);
    if (found_system != _connected_or_discovered_systems.cend()) {
        auto parts = found_system->second.getParticipants();
        for (const auto& part: parts) {
            if (part.getName().compare(0u, word_prefix.size(), word_prefix) == 0) {
                completions.push_back(part.getName());
            }
        }
    }
    return completions;
}

std::string resolveFilesystemErrorCode(const a_util::filesystem::Error error_code)
{
    switch (error_code) {
    case a_util::filesystem::OK:
        return "OK";
    case a_util::filesystem::OPEN_FAILED:
        return "OPEN_FAILED";
    case a_util::filesystem::GENERAL_FAILURE:
        return "GENERAL_FAILURE";
    case a_util::filesystem::IO_ERROR:
        return "IO_ERROR";
    case a_util::filesystem::INVALID_PATH:
        return "INVALID_PATH";
    case a_util::filesystem::ACCESS_DENIED:
        return "ACCESS_DENIED";
    default:
        assert(false);
    }
    return "UNKNOWN_ERROR";
}

class JsonObject {
    Json::Value jsonValue;

public:
    JsonObject(const std::string& action, CmdStatus cmdStatus = CmdStatus::no_error)
    {
        jsonValue["action"] = action;
        jsonValue["status"] =
            static_cast<typename std::underlying_type<CmdStatus>::type>(cmdStatus);
    }
    void setValue(const std::string& type, const std::string& value)
    {
        jsonValue[type] = value;
    }
    const Json::Value& getObject()
    {
        return jsonValue;
    }
};

// writes a simple json object with 'action' and 'note'
// 'note' contains an arbitrary string with any information
// on 'disableJson', only the content of 'note' will be written
void FepControl::writeNote(const std::string& action, const std::string& note)
{
    if (_json_mode) {
        JsonObject jsonObject(action);
        jsonObject.setValue("note", note);
        writeOutput(_builder.convertJson(jsonObject.getObject()), "\n");
    }
    else {
        writeOutput(note, "\n");
    }
}

// writes a json object with 'action' and another attribute
// 'attribute' contains most time a list of elements separated by ','
// on 'disableJson', attribute and values will be written, separated by ':'
void FepControl::writeNote(const std::string& action,
                           const std::pair<std::string, std::string>& attribute)
{
    if (_json_mode) {
        JsonObject jsonObject(action);
        jsonObject.setValue(attribute.first, attribute.second);
        writeOutput(_builder.convertJson(jsonObject.getObject()), "\n");
    }
    else {
        writeOutput(attribute.first, " : ", attribute.second, "\n");
    }
}

// writes a json object with 'action' and arbitrary attributes
// each pair of attribute / value will be written in a separate line
// on 'disableJson', a list of all values will be written, separated by ':'
void FepControl::writeNotes(const std::string& action,
                            const std::vector<std::pair<std::string, std::string>>& attributes)
{
    if (_json_mode) {
        JsonObject jsonObject(action);
        for (const auto& attribute: attributes) {
            jsonObject.setValue(attribute.first, attribute.second);
        }
        writeOutput(_builder.convertJson(jsonObject.getObject()), "\n");
    }
    else {
        std::vector<std::string> notes;
        for (const auto& attribute: attributes) {
            notes.push_back(attribute.second);
        }
        writeOutput(a_util::strings::join(notes, " : "), "\n");
    }
}

// writes a simple json object with 'action', 'error' and optional 'reason'
// 'error' contains an arbitrary string with advanced information to the error
// on 'disableJson', only 'error' and 'reason' will be written, separated by ','
void FepControl::writeError(const std::string& action,
                            const std::string& error,
                            const CmdStatus status,
                            const std::string& reason = "")
{
    if (_json_mode) {
        JsonObject jsonObject(action, status);
        jsonObject.setValue("error", error);
        if (reason != "") {
            jsonObject.setValue("reason", reason);
        }
        writeOutput(_builder.convertJson(jsonObject.getObject()), "\n");
    }
    else {
        writeOutput(error);
        if (reason != "") {
            writeOutput(", error: ", reason);
        }
        writeOutput("\n");
    }
}

// writes a simple json object with 'action', 'exception' and optional 'reason'
// behavior is equivalent to writeError, only parameters of function are different
void FepControl::writeException(const std::string& action,
                                const std::string& exception,
                                const CmdStatus status,
                                const std::exception& e)
{
    if (_json_mode) {
        JsonObject jsonObject(action, status);
        jsonObject.setValue("exception", exception);
        jsonObject.setValue("reason", e.what());
        writeOutput(_builder.convertJson(jsonObject.getObject()), "\n");
    }
    else {
        writeOutput(exception, ", exception: ", e.what(), "\n");
    }
}

void FepControl::dumpSystemParticipants(const std::string& action, const fep3::System& system)
{
    std::string system_name = system.getSystemName();
    if (system_name.empty()) {
        system_name = _empty_system_name;
    }
    auto participants = system.getParticipants();
    std::vector<std::string> names;
    for (const auto& participant: participants) {
        names.push_back(participant.getName());
    }
    const std::vector<std::pair<std::string, std::string>> attributes{
        std::make_pair("system name", system_name),
        std::make_pair("participants", a_util::strings::join(names, ", ")),
    };
    writeNotes(action, attributes);
}

bool FepControl::discoverAllSystems(TokenIterator first, TokenIterator)
{
    auto systems = fep3::discoverAllSystems();
    for (auto& system: systems) {
        dumpSystemParticipants(*first, system);
        std::string system_name = system.getSystemName();
        if (system_name.empty()) {
            // special system name -
            system_name = _empty_system_name;
        }
        _connected_or_discovered_systems[system_name] = std::move(system);
        // this updates for completion
        _last_system_name_used = system_name;
    }
    return true;
}

bool FepControl::discoverSystem(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    std::string system_name = *first;
    // this updates for completion
    _last_system_name_used = system_name;
    if (system_name == _empty_system_name) {
        system_name = "";
    }
    auto system = fep3::discoverSystem(system_name);

    dumpSystemParticipants(action, system);
    _connected_or_discovered_systems[system.getSystemName()] = std::move(system);
    return true;
}

bool FepControl::setCurrentWorkingDirectory(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    auto path = a_util::filesystem::Path(*first);
    path.makeCanonical();
    auto result = a_util::filesystem::setWorkingDirectory(path);
    if (result == a_util::filesystem::OK) {
        writeNote(action,
                  std::pair<std::string, std::string>("working directory", path.toString()));
        return true;
    }
    const std::string error = "cannot set working directory to '" + path.toString() + "'";
    writeError(action, error, CmdStatus::filesystem_error, resolveFilesystemErrorCode(result));
    return false;
}

bool FepControl::getCurrentWorkingDirectory(TokenIterator first, TokenIterator)
{
    auto current_path = a_util::filesystem::getWorkingDirectory();
    writeNote(*first,
              std::pair<std::string, std::string>("working directory", current_path.toString()));
    return true;
}

bool FepControl::setPriority(TokenIterator first, TokenIterator, const PriorityType type)
{
    const std::string action = *(first);
    const std::string system = *(++first);
    const std::string participant = *(++first);
    const std::string priority = *(++first);

    auto it = getConnectedOrDiscoveredSystem(system, _auto_discovery_of_systems);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto part = it->second.getParticipant(participant);
        if (part) {
            switch (type) {
            case PriorityType::init_priority:
                part.setInitPriority(std::stoi(priority));
                break;
            case PriorityType::start_priority:
                part.setStartPriority(std::stoi(priority));
                break;
            default:
                assert(false);
            }
            writeNote(action, "priority for '" + participant + "' set");
            return true;
        }
        // participant does not exist
        const std::string error =
            "participant '" + participant + "' is not in system '" + system + "'";
        writeError(action, error, CmdStatus::participant_error);
        return false;
    }
    catch (const std::exception& e) {
        const std::string exception =
            "cannot set priority for '" + participant + "@" + system + "'";
        writeException(action, exception, CmdStatus::generic_error, e);
        return false;
    }
}

bool FepControl::getPriority(TokenIterator first, TokenIterator, const PriorityType type)
{
    const std::string action = *(first);
    const std::string system = *(++first);
    const std::string participant = *(++first);
    int32_t priority = 0;

    auto it = getConnectedOrDiscoveredSystem(system, _auto_discovery_of_systems);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto part = it->second.getParticipant(participant);
        if (part) {
            switch (type) {
            case PriorityType::init_priority:
                priority = part.getInitPriority();
                break;
            case PriorityType::start_priority:
                priority = part.getStartPriority();
                break;
            default:
                assert(false);
            }
            writeNote(action, std::make_pair("priority", std::to_string(priority)));
            return true;
        }
        // participant does not exist
        const std::string error =
            "participant '" + participant + "' is not in system '" + system + "'";
        writeError(action, error, CmdStatus::participant_error);
        return false;
    }
    catch (const std::exception& e) {
        const std::string exception =
            "cannot get priority for '" + participant + "@" + system + "'";
        writeException(action, exception, CmdStatus::generic_error, e);
        return false;
    }
}

bool FepControl::setInitPriority(TokenIterator first, TokenIterator last)
{
    return setPriority(first, last, PriorityType::init_priority);
}

bool FepControl::getInitPriority(TokenIterator first, TokenIterator last)
{
    return getPriority(first, last, PriorityType::init_priority);
}

bool FepControl::setStartPriority(TokenIterator first, TokenIterator last)
{
    return setPriority(first, last, PriorityType::start_priority);
}

bool FepControl::getStartPriority(TokenIterator first, TokenIterator last)
{
    return getPriority(first, last, PriorityType::start_priority);
}

bool FepControl::changeStateMethod(TokenIterator first,
                                   std::function<void(fep3::System& system)> call,
                                   const std::string& action,
                                   const std::string& success_message,
                                   const std::string& failed_message)
{
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        call(it->second);
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot " + failed_message + " system '" + *first + "'";
        writeException(action, exception, CmdStatus::generic_error, e);
        return false;
    }
    const std::string note = *first + " " + success_message;
    writeNote(action, note);
    return true;
}

bool FepControl::startSystem(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return changeStateMethod(
        first, [](fep3::System& sys) { sys.start(); }, action, "started", "start");
}
bool FepControl::stopSystem(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return changeStateMethod(
        first, [](fep3::System& sys) { sys.stop(); }, action, "stopped", "stop");
}

bool FepControl::loadSystem(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return changeStateMethod(
        first, [](fep3::System& sys) { sys.load(); }, action, "loaded", "load");
}
bool FepControl::unloadSystem(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return changeStateMethod(
        first, [](fep3::System& sys) { sys.unload(); }, action, "unloaded", "unload");
}

bool FepControl::initializeSystem(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return changeStateMethod(
        first, [](fep3::System& sys) { sys.initialize(); }, action, "initialized", "initialize");
}

bool FepControl::deinitializeSystem(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return changeStateMethod(
        first,
        [](fep3::System& sys) { sys.deinitialize(); },
        action,
        "deinitialized",
        "deinitialize");
}
bool FepControl::pauseSystem(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return changeStateMethod(
        first, [](fep3::System& sys) { sys.pause(); }, action, "paused", "pause");
}

bool FepControl::shutdownSystem(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return changeStateMethod(
        first,
        [&](fep3::System& sys) {
            const std::string name = sys.getSystemName();
            sys.shutdown();
            _connected_or_discovered_systems.erase(name);
        },
        action,
        "shutdowned",
        "shutdown");
}

bool FepControl::startMonitoringSystem(TokenIterator first, TokenIterator)
{
    const std::string action = *first;

    auto it = getConnectedOrDiscoveredSystem(*(++first), _auto_discovery_of_systems);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    else {
        try {
            it->second.unregisterMonitoring(monitor);
        }
        catch (const std::exception&) {
            //...
        }
        it->second.registerMonitoring(monitor);
        writeNote(action, "monitoring: enabled");
        return true;
    }
}

bool FepControl::stopMonitoringSystem(TokenIterator first, TokenIterator)
{
    const std::string action = *first;

    auto it = getConnectedOrDiscoveredSystem(*(++first), _auto_discovery_of_systems);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    else {
        try {
            it->second.unregisterMonitoring(monitor);
        }
        catch (const std::exception&) {
            //...
        }
        writeNote(action, "monitoring: disabled");
        return true;
    }
}

bool FepControl::doParticipantStateChange(
    TokenIterator& first,
    std::function<void(fep3::RPCComponent<fep3::rpc::IRPCParticipantStateMachine>&)> change_state,
    const std::string& action,
    const std::string& message_1,
    const std::string& message_2)
{
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    std::string partname = "";
    try {
        partname = *std::next(first);
        auto part = it->second.getParticipant(partname);
        if (part) {
            auto state_machine =
                part.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>();
            if (state_machine) {
                change_state(state_machine);
            }
            else {
                const std::string error =
                    "participant '" + partname + "@" + *first + "' has no state machine";
                writeError(action, error, CmdStatus::statechange_error);
                return false;
            }
        }
        else {
            const std::string error =
                "participant '" + partname + "' is not in system '" + *first + "'";
            writeError(action, error, CmdStatus::statechange_error);
            return false;
        }

        // this updates for completion
        _last_system_name_used = it->first;
    }
    catch (const std::exception& e) {
        const std::string exception =
            "cannot " + message_1 + " participant '" + partname + "@" + *first + "'";
        writeException(action, exception, CmdStatus::statechange_error, e);
        return false;
    }
    const std::string note = partname + "@" + *first + " " + message_2;
    writeNote(action, note);
    return true;
}

bool FepControl::loadParticipant(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return doParticipantStateChange(
        first,
        [](fep3::RPCComponent<fep3::rpc::IRPCParticipantStateMachine>& part) { part->load(); },
        action,
        "load",
        "loaded");
}
bool FepControl::unloadParticipant(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return doParticipantStateChange(
        first,
        [](fep3::RPCComponent<fep3::rpc::IRPCParticipantStateMachine>& part) { part->unload(); },
        action,
        "unload",
        "unloaded");
}

bool FepControl::initializeParticipant(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return doParticipantStateChange(
        first,
        [](fep3::RPCComponent<fep3::rpc::IRPCParticipantStateMachine>& part) {
            part->initialize();
        },
        action,
        "initialize",
        "initialized");
}

bool FepControl::deinitializeParticipant(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return doParticipantStateChange(
        first,
        [](fep3::RPCComponent<fep3::rpc::IRPCParticipantStateMachine>& part) {
            part->deinitialize();
        },
        action,
        "deinitialize",
        "deinitialized");
}

bool FepControl::startParticipant(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return doParticipantStateChange(
        first,
        [](fep3::RPCComponent<fep3::rpc::IRPCParticipantStateMachine>& part) { part->start(); },
        action,
        "start",
        "started");
}
bool FepControl::stopParticipant(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return doParticipantStateChange(
        first,
        [](fep3::RPCComponent<fep3::rpc::IRPCParticipantStateMachine>& part) { part->stop(); },
        action,
        "stop",
        "stopped");
}

bool FepControl::pauseParticipant(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return doParticipantStateChange(
        first,
        [](fep3::RPCComponent<fep3::rpc::IRPCParticipantStateMachine>& part) { part->pause(); },
        action,
        "pause",
        "paused");
}

bool FepControl::shutdownParticipant(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    return doParticipantStateChange(
        first,
        [](fep3::RPCComponent<fep3::rpc::IRPCParticipantStateMachine>& part) { part->shutdown(); },
        action,
        "shutdown",
        "shutdowned");
}

bool FepControl::getSystemState(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto state = it->second.getSystemState();
        const std::vector<std::pair<std::string, std::string>> attributes{
            std::make_pair("stateID", std::to_string(state._state)),
            std::make_pair("stateName", resolveSystemState(state._state)),
            std::make_pair("homogeneous", state._homogeneous ? "homogeneous" : "inhomogeneous"),
        };
        writeNotes(action, attributes);
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot get system state for '" + *first + "'";
        writeException(action, exception, CmdStatus::generic_error, e);
        return false;
    }
    return true;
}

bool FepControl::setSystemState(TokenIterator first, TokenIterator last)
{
    const std::string action = *(first++);
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    std::string state_string = *std::next(first);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto state_to_set = getStateFromString(state_string);
        if (state_to_set == fep3::SystemAggregatedState::unreachable) {
            it->second.setSystemState(fep3::SystemAggregatedState::unloaded);
            return shutdownSystem(--first, last);
        }
        else {
            it->second.setSystemState(state_to_set);
            getSystemState(--first, last);
        }
    }
    catch (const std::exception& e) {
        const std::string exception =
            "cannot set system state '" + state_string + "' for '" + *first + "'";
        writeException(action, exception, CmdStatus::generic_error, e);
        return false;
    }
    return true;
}

bool FepControl::getParticipants(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    dumpSystemParticipants(action, it->second);
    return true;
}

bool FepControl::quit(TokenIterator, TokenIterator)
{
    writeOutput("bye bye", "\n");
    // we clear that here before any static variable is closed
    _connected_or_discovered_systems.clear();
    exit(0);
}

bool FepControl::configureSystemTimingSystemTime(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    const std::string master_name = *std::next(first);
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        it->second.configureTiming3ClockSyncOnlyInterpolation(master_name, "100");
        writeNote(action, "successfully set SystemTimingSystemTime");
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot set timing for '" + *first + "'";
        writeException(action, exception, CmdStatus::systemtiming_error, e);
        return false;
    }
    return true;
}

bool FepControl::configureSystemTimingDiscrete(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    const std::string system_name = *first;
    const std::string master_name = *(++first);
    const std::string factor = *(++first);
    const std::string step_size = *(++first);
    auto it = getConnectedOrDiscoveredSystem(system_name, _auto_discovery_of_systems);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        it->second.configureTiming3DiscreteSteps(master_name, step_size, factor);
        writeNote(action, "successfully set SystemTimingDiscrete");
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot set timing for '" + system_name + "'";
        writeException(action, exception, CmdStatus::systemtiming_error, e);
        return false;
    }
    return true;
}

bool FepControl::configureSystemTimeNoSync(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    const std::string system_name = *first;
    auto it = getConnectedOrDiscoveredSystem(system_name, _auto_discovery_of_systems);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        // this updates for completion
        _last_system_name_used = system_name;
        it->second.configureTiming3NoMaster();
        writeNote(action, "successfully set SystemTimeNoSync");
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot set timing for '" + system_name + "'";
        writeException(action, exception, CmdStatus::systemtiming_error, e);
        return false;
    }
    return true;
}

bool FepControl::getCurrentTimingMaster(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    const std::string system_name = *first;

    auto it = getConnectedOrDiscoveredSystem(system_name, _auto_discovery_of_systems);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto masters = it->second.getCurrentTimingMasters();
        auto masters_string = a_util::strings::join(masters, ",");
        writeNote(action, std::pair<std::string, std::string>("timing masters", masters_string));
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot get timing master for '" + system_name + "'";
        writeException(action, exception, CmdStatus::systemtiming_error, e);
        return false;
    }
    return true;
}

bool FepControl::enableAutoDiscovery(TokenIterator first, TokenIterator)
{
    _auto_discovery_of_systems = true;
    writeNote(*first, "auto_discovery: enabled");
    return true;
}

bool FepControl::disableAutoDiscovery(TokenIterator first, TokenIterator)
{
    _auto_discovery_of_systems = false;
    writeNote(*first, "auto_discovery: disabled");
    return true;
}

bool FepControl::enableJsonMode(TokenIterator first, TokenIterator)
{
    _json_mode = true;
    monitor.setJsonMode(true);
    writeNote(*first, "json_mode: enabled");
    return true;
}

bool FepControl::disableJsonMode(TokenIterator first, TokenIterator)
{
    _json_mode = false;
    monitor.setJsonMode(false);
    writeNote(*first, "json_mode: disabled");
    return true;
}

bool FepControl::getParticipantState(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    const std::string system_name = *(first);
    auto it = getConnectedOrDiscoveredSystem(system_name, _auto_discovery_of_systems);
    const std::string participant_name = *std::next(first);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto part = it->second.getParticipant(participant_name);
        if (part) {
            auto state_machine =
                part.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>();
            if (state_machine) {
                auto value = state_machine->getState();
                const std::vector<std::pair<std::string, std::string>> attributes{
                    std::make_pair("stateID", std::to_string(value)),
                    std::make_pair("stateName", resolveSystemState(value)),
                };
                writeNotes(action, attributes);
            }
            else {
                const std::string error = "participant '" + participant_name + "@" + system_name +
                                          "' has no state machine";
                writeError(action, error, CmdStatus::participant_error);
                return false;
            }
        }
        else {
            const std::string error =
                "participant '" + participant_name + "' is not in system '" + *first + "'";
            writeError(action, error, CmdStatus::participant_error);
            return false;
        }
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot get participant state for participant '" +
                                      participant_name + "@" + *first + "'";
        writeException(action, exception, CmdStatus::participant_error, e);
        return false;
    }
    return true;
}

bool FepControl::setParticipantState(TokenIterator first, TokenIterator last)
{
    const std::string action = *(first++);
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    const std::string system_name = *first;
    const std::string participant_name = *(++first);
    const std::string state_string = *(++first);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto part = it->second.getParticipant(participant_name);
        if (part) {
            // create a second temporary system with only one participant
            fep3::System system_temp(system_name);
            system_temp.add(participant_name);

            auto state_to_set = getStateFromString(state_string);
            if (state_to_set == fep3::SystemAggregatedState::unreachable) {
                system_temp.setSystemState(fep3::SystemAggregatedState::unloaded);
                system_temp.shutdown();
            }
            else {
                system_temp.setSystemState(state_to_set);
            }
            const std::vector<std::pair<std::string, std::string>> attributes{
                std::make_pair("stateID", std::to_string(state_to_set)),
                std::make_pair("stateName", resolveSystemState(state_to_set))};
            writeNotes(action, attributes);
        }
        else {
            const std::string error =
                "participant '" + participant_name + "' is not in system '" + system_name + "'";
            writeError(action, error, CmdStatus::participant_error);
            return false;
        }
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot set participant state" + state_string +
                                      "for participant '" + participant_name + "@" + system_name +
                                      "'";
        writeException(action, exception, CmdStatus::participant_error, e);
        return false;
    }
    return true;
}

template <typename T>
void traverseProperties(fep3::RPCComponent<fep3::rpc::IRPCConfiguration> conf,
                        const std::string& node,
                        const std::string& prop_name,
                        std::function<T*(const std::string& name,
                                         const std::string& value,
                                         const std::string& type,
                                         const int depth,
                                         T* parent_user_object)> callback,
                        T* user_object,
                        const int depth = 0)
{
    T* result_user_object = user_object;
    if (depth != 0 || (!node.empty() && !prop_name.empty())) {
        auto prop = conf->getProperties(node);
        auto value = prop->getProperty(prop_name);
        auto type = prop->getPropertyType(prop_name);
        result_user_object = callback(prop_name, value, type, depth, result_user_object);
    }

    std::string next_node = node;
    if (next_node.find_last_of("/") != next_node.length() - 1 && !prop_name.empty()) {
        next_node = next_node.append("/");
    }

    auto props = conf->getProperties(next_node.append(prop_name));
    auto sub_props = props->getPropertyNames();
    for (auto sub_prop: sub_props) {
        traverseProperties<T>(conf, next_node, sub_prop, callback, result_user_object, depth + 1);
    }
}

Json::Value formatPropertyJson(const std::string& action,
                               fep3::RPCComponent<fep3::rpc::IRPCConfiguration> conf,
                               const std::string& node,
                               const std::string& prop_name,
                               int indent = 0)
{
    Json::Value root;

    traverseProperties<Json::Value>(
        conf,
        node,
        prop_name,
        [](const std::string& name,
           const std::string& value,
           const std::string& type,
           const int depth,
           Json::Value* parent_json_value) -> Json::Value* {
            if (depth == 0) {
                (*parent_json_value)["name"] = name;
                (*parent_json_value)["value"] = value;
                (*parent_json_value)["type"] = type;
                return parent_json_value;
            }
            else {
                Json::Value prop;
                prop["name"] = name;
                prop["value"] = value;
                prop["type"] = type;
                return &(*parent_json_value)["sub_properties"].append(prop);
            }
        },
        &root);

    if (node.empty() && prop_name.empty()) {
        return root["sub_properties"];
    }
    else {
        return root;
    }
}

std::string formatProperty(fep3::RPCComponent<fep3::rpc::IRPCConfiguration> conf,
                           const std::string& node,
                           const std::string& prop_name,
                           int indent = 0)
{
    std::stringstream ss;
    traverseProperties<std::stringstream>(
        conf,
        node,
        prop_name,
        [](const std::string& name,
           const std::string& value,
           const std::string& type,
           const int depth,
           std::stringstream* ss) -> std::stringstream* {
            *ss << std::string(depth * 2, ' ') << name << " : " << value << std::endl;
            return ss;
        },
        &ss);

    return ss.str().c_str();
}

static std::pair<std::string, std::string> getNodeAndLeafNamefromProperty(
    const std::string& prop_path)
{
    auto pos_name = prop_path.find_last_of("/");
    auto node = prop_path.substr(0, pos_name);
    auto name = (pos_name == std::string::npos) ? "" : prop_path.substr(pos_name + 1);
    return std::make_pair(node, name);
}

bool FepControl::getParticipantPropertyNames(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    const std::string participant_name = *std::next(first);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }

    try {
        auto part = it->second.getParticipant(participant_name);
        if (part) {
            auto conf = part.getRPCComponentProxy<fep3::rpc::IRPCConfiguration>();
            if (conf) {
                if (_json_mode) {
                    Json::Value root;
                    root["action"] = action;
                    root["status"] = static_cast<typename std::underlying_type<CmdStatus>::type>(
                        CmdStatus::no_error);

                    traverseProperties<Json::Value>(
                        conf,
                        "",
                        "",
                        [](const std::string& name,
                           const std::string& value,
                           const std::string& type,
                           int depth,
                           Json::Value* parent_json) -> Json::Value* {
                            Json::Value prop;
                            prop["name"] = name;
                            prop["type"] = type;
                            return &(*parent_json)["sub_properties"].append(prop);
                        },
                        &root);
                    writeOutput(_builder.convertJson(root), "\n");
                }
                else {
                    std::stringstream ss;
                    traverseProperties<std::stringstream>(
                        conf,
                        "",
                        "",
                        [](const std::string& name,
                           const std::string& value,
                           const std::string& type,
                           int depth,
                           std::stringstream* ss) -> std::stringstream* {
                            *ss << std::string(depth * 2, ' ') << name << std::endl;
                            return ss;
                        },
                        &ss);
                    writeOutput("property names :", "\n", ss.str(), "\n");
                }
            }
        }
        else {
            const std::string error =
                "participant '" + participant_name + "' is not in system '" + *first + "'";
            writeError(action, error, CmdStatus::participant_error);
            return false;
        }
    }

    catch (const std::exception& e) {
        const std::string exception =
            "cannot get property names for participant '" + participant_name + "@" + *first + "'";
        writeException(action, exception, CmdStatus::participant_error, e);
        return false;
    }
    return true;
}

bool FepControl::getParticipantProperties(TokenIterator first, TokenIterator)
{
    const std::string action = *(first++);
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    const std::string system_name = *first;
    const std::string participant_name = *std::next(first);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto part = it->second.getParticipant(participant_name);
        if (part) {
            auto conf = part.getRPCComponentProxy<fep3::rpc::IRPCConfiguration>();
            if (conf) {
                if (_json_mode) {
                    Json::Value participant_properties = formatPropertyJson(action, conf, "", "");

                    Json::Value output;
                    output["action"] = action;
                    output["status"] = static_cast<typename std::underlying_type<CmdStatus>::type>(
                        CmdStatus::no_error);
                    output["participant"] = participant_name;
                    output["participant_properties"] = participant_properties;

                    writeOutput(_builder.convertJson(output), "\n");
                }
                else {
                    writeOutput(participant_name, " : ", "\n", formatProperty(conf, "", ""), "\n");
                }
            }
            else {
                const std::string error =
                    "participant '" + participant_name + "@" + system_name + "' has no RPC Info";
                writeError(action, error, CmdStatus::rpcobject_error);
                return false;
            }
        }
        else {
            const std::string error =
                "participant '" + participant_name + "' is not in system '" + system_name + "'";
            writeError(action, error, CmdStatus::participant_error);
            return false;
        }
    }
    catch (const std::exception& e) {
        const std::string exception =
            "cannot get properties for participant '" + participant_name + "@" + system_name + "'";
        writeException(action, exception, CmdStatus::participant_error, e);
        return false;
    }
    return true;
}

bool FepControl::getParticipantProperty(TokenIterator first, TokenIterator last)
{
    const std::string action = *(first++);
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    const std::string system_name = *first;
    const std::string participant_name = *(++first);
    const std::string property_path = *(++first);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto part = it->second.getParticipant(participant_name);
        if (part) {
            auto conf = part.getRPCComponentProxy<fep3::rpc::IRPCConfiguration>();
            if (conf) {
                auto split_path = getNodeAndLeafNamefromProperty(property_path);
                auto node = split_path.first;
                auto leaf_name = split_path.second;

                if (_json_mode) {
                    Json::Value participant_property =
                        formatPropertyJson(action, conf, node, leaf_name);

                    Json::Value output;
                    output["action"] = action;
                    output["status"] = static_cast<typename std::underlying_type<CmdStatus>::type>(
                        CmdStatus::no_error);
                    output["name"] = participant_property["name"];
                    output["value"] = participant_property["value"];
                    output["type"] = participant_property["type"];
                    writeOutput(_builder.convertJson(output), "\n");
                }
                else {
                    writeOutput(formatProperty(conf, node, leaf_name), "\n");
                }

                // append to used props, if not used already
                if (std::find(_used_properties.begin(), _used_properties.end(), participant_name) !=
                    _used_properties.end()) {
                    _used_properties.push_back(property_path);
                }
            }
            else {
                const std::string error = "participant '" + participant_name + "@" + system_name +
                                          "' has no RPC configuration";
                writeError(action, error, CmdStatus::participant_error);
                return false;
            }
        }
        else {
            const std::string error =
                "participant '" + participant_name + "' is not in system '" + system_name + "'";
            writeError(action, error, CmdStatus::participant_error);
            return false;
        }
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot get property " + property_path +
                                      " for participant '" + participant_name + "@" + system_name +
                                      "'";
        writeException(action, exception, CmdStatus::participant_error, e);
        return false;
    }
    return true;
}

bool FepControl::setParticipantProperty(TokenIterator first, TokenIterator last)
{
    const std::string action = *(first++);
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    const std::string system_name = *first;
    const std::string participant_name = *(++first);
    const std::string property_path = *(++first);
    std::string property_value = *(++first);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto part = it->second.getParticipant(participant_name);
        if (part) {
            auto conf = part.getRPCComponentProxy<fep3::rpc::IRPCConfiguration>();
            if (conf) {
                auto split_path = getNodeAndLeafNamefromProperty(property_path);
                auto node = split_path.first;
                auto leaf_name = split_path.second;

                auto prop = conf->getProperties(node);
                if (prop) {
                    prop->setProperty(leaf_name, property_value, prop->getPropertyType(leaf_name));
                    // append to used props, if not used already
                    if (std::find(_used_properties.begin(),
                                  _used_properties.end(),
                                  participant_name) != _used_properties.end()) {
                        _used_properties.push_back(property_path);
                    }
                    writeNote(action, "property set");
                }
                else {
                    const std::string error = "participant '" + participant_name + "@" +
                                              system_name + "' has no property" + node;
                    writeError(action, error, CmdStatus::participant_error);
                    return false;
                }
            }
            else {
                const std::string error = "participant '" + participant_name + "@" + system_name +
                                          "' has no RPC configuration";
                writeError(action, error, CmdStatus::participant_error);
                return false;
            }
        }
        else {
            const std::string error =
                "participant '" + participant_name + "' is not in system '" + system_name + "'";
            writeError(action, error, CmdStatus::participant_error);
            return false;
        }
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot set property" + property_path + " for participant '" +
                                      participant_name + "@" + system_name + "'";
        writeException(action, exception, CmdStatus::participant_error, e);
        return false;
    }
    return true;
}

bool FepControl::getRPCObjectsParticipant(TokenIterator first, TokenIterator last)
{
    const std::string action = *(first++);
    const std::string system_name = *(first);
    auto it = getConnectedOrDiscoveredSystem(system_name, _auto_discovery_of_systems);
    const std::string participant_name = *std::next(first);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto part = it->second.getParticipant(participant_name);
        if (part) {
            auto info = part.getRPCComponentProxy<fep3::rpc::IRPCParticipantInfo>();
            if (info) {
                auto value = info->getRPCComponents();
                writeNote(action,
                          std::pair<std::string, std::string>("names",
                                                              a_util::strings::join(value, ", ")));
            }
            else {
                const std::string error =
                    "participant '" + participant_name + "@" + system_name + "' has no RPC Info";
                writeError(action, error, CmdStatus::rpcobject_error);
                return false;
            }
        }
        else {
            const std::string error =
                "participant '" + participant_name + "' is not in system '" + system_name + "'";
            writeError(action, error, CmdStatus::rpcobject_error);
            return false;
        }
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot get participant state for participant '" +
                                      participant_name + "@" + system_name + "'";
        writeException(action, exception, CmdStatus::rpcobject_error, e);
        return false;
    }
    return true;
}

bool FepControl::getRPCObjectIIDSParticipant(TokenIterator first, TokenIterator last)
{
    const std::string action = *(first++);
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    const std::string system_name = *(first);
    const std::string participant_name = *(++first);
    const std::string object_name = *(++first);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto part = it->second.getParticipant(participant_name);
        if (part) {
            auto info = part.getRPCComponentProxy<fep3::rpc::IRPCParticipantInfo>();
            if (info) {
                try {
                    auto value = info->getRPCComponentIIDs(object_name);
                    writeNote(action,
                              std::pair<std::string, std::string>(
                                  "identifiers", a_util::strings::join(value, ", ")));
                }
                catch (const std::exception&) {
                    const std::string exception = "participant '" + participant_name + "@" +
                                                  system_name + "' IID info can not be retrieved";
                    writeError(action, exception, CmdStatus::rpcobject_error);
                    return false;
                }
            }
            else {
                const std::string error =
                    "participant '" + participant_name + "@" + system_name + "' has no RPC Info";
                writeError(action, error, CmdStatus::rpcobject_error);
                return false;
            }
        }
        else {
            const std::string error =
                "participant '" + participant_name + "' is not in system '" + system_name + "'";
            writeError(action, error, CmdStatus::rpcobject_error);
            return false;
        }
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot get participant state for participant '" +
                                      participant_name + "@" + system_name + "'";
        writeException(action, exception, CmdStatus::rpcobject_error, e);
        return false;
    }
    return true;
}

bool FepControl::getRPCObjectDefinitionParticipant(TokenIterator first, TokenIterator last)
{
    const std::string action = *(first++);
    auto it = getConnectedOrDiscoveredSystem(*first, _auto_discovery_of_systems);
    const std::string system_name = *(first);
    const std::string participant_name = *(++first);
    const std::string object_name = *(++first);
    const std::string intf_name = *(++first);
    if (it == _connected_or_discovered_systems.end()) {
        return false;
    }
    try {
        auto part = it->second.getParticipant(participant_name);
        if (part) {
            auto info = part.getRPCComponentProxy<fep3::rpc::IRPCParticipantInfo>();
            if (info) {
                try {
                    auto value = info->getRPCComponentInterfaceDefinition(object_name, intf_name);
                    writeNote(action, std::pair<std::string, std::string>("definition", value));
                }
                catch (const std::exception& e) {
                    const std::string exception = "participant '" + participant_name + "@" +
                                                  system_name + "' IID info can not be retrieved";
                    writeException(action, exception, CmdStatus::rpcobject_error, e);
                    return false;
                }
            }
            else {
                const std::string error =
                    "participant '" + participant_name + "@" + system_name + "' has no RPC Info";
                writeError(action, error, CmdStatus::rpcobject_error);
                return false;
            }
        }
        else {
            const std::string error =
                "participant '" + participant_name + "' is not in system '" + system_name + "'";
            writeError(action, error, CmdStatus::rpcobject_error);
            return false;
        }
    }
    catch (const std::exception& e) {
        const std::string exception = "cannot get participant state for participant '" +
                                      participant_name + "@" + system_name + "'";
        writeException(action, exception, CmdStatus::rpcobject_error, e);
        return false;
    }
    return true;
}

std::vector<ControlCommand>::const_iterator FepControl::findCommand(
    const std::string& command_candidate)
{
    return std::find_if(
        getControlCommands().begin(),
        getControlCommands().end(),
        [&command_candidate](const ControlCommand& cmd) { return cmd._name == command_candidate; });
}

std::vector<std::string> FepControl::commandNameCompletion(const std::string& word_prefix)
{
    std::vector<std::string> completions;
    for (const auto& cmd: getControlCommands()) {
        const std::string& cmd_name = cmd._name;
        if (cmd_name.compare(0u, word_prefix.size(), word_prefix) == 0) {
            completions.push_back(cmd_name);
        }
    }
    return completions;
}

bool FepControl::help(TokenIterator first, TokenIterator last)
{
    std::stringstream output;

    if (++first == last) {
        for (const auto& cmd: getControlCommands()) {
            if (cmd._hidden) {
                continue;
            }

            // writeOutput(cmd._name , " : " , cmd._description , "\n";
            output << cmd._name << " : " << cmd._description << "\n";
        }
    }
    else {
        auto it = findCommand(*first);
        if (it == getControlCommands().end()) {
            output << "no such command as \"" << *first << "\""
                   << "\n";
            return false;
        }
        output << *first;
        for (const auto& argument: it->_arguments) {
            output << " <" << argument._description << '>';
        }
        output << " : " << it->_description << "\n";
    }

    writeOutput(output.str());

    return true;
}

int FepControl::processCommandline(const std::vector<std::string>& command_line)
{
    assert(!command_line.empty());
    auto it = findCommand(command_line[0]);
    if (it == getControlCommands().end()) {
        const std::string error =
            "Invalid command '" + command_line[0] + "', use 'help' for valid commands";
        writeError("processCommandline", error, CmdStatus::input_error);
        return -2;
    }
    if (command_line.size() > (*it)._arguments.size() + 1u ||
        command_line.size() < (*it)._arguments.size() + 1u - (*it)._last_optional_parameters) {
        std::string error = "Invalid number of arguments for '" + command_line[0] + "' (" +
                            std::to_string(command_line.size() - 1u) + " instead of ";
        if ((*it)._last_optional_parameters == 0u) {
            error += std::to_string((*it)._arguments.size());
        }
        else {
            error += std::to_string((*it)._arguments.size() - (*it)._last_optional_parameters) +
                     ".." + std::to_string((*it)._arguments.size());
        }
        error += "), use 'help' for more information";
        writeError("processCommandline", error, CmdStatus::input_error);
        return -3;
    }

    auto func = std::bind((*it)._action, this, command_line.begin(), command_line.end());

    return func() ? 0 : 1;
}

std::vector<std::string> FepControl::possibleSystemsStateCompletion(const std::string& word_prefix)
{
    std::vector<std::string> completions;
    std::vector<std::string> possible_states = {
        "shutdowned", "unloaded", "loaded", "initialized", "paused", "running"};
    for (const auto& state_val: possible_states) {
        if (state_val.compare(0u, word_prefix.size(), word_prefix) == 0) {
            completions.push_back(state_val);
        }
    }
    return completions;
}

const std::vector<ControlCommand>& FepControl::getControlCommands() const noexcept
{
    static const std::vector<ControlCommand> commands{
        ControlCommand{"exit", "quits this program", &FepControl::quit, {}, 0u},
        ControlCommand{"quit", "quits this program", &FepControl::quit, {}, 0u},
        ControlCommand{"discoverAllSystems",
                       "discovers all systems and registers logging monitor for them",
                       &FepControl::discoverAllSystems,
                       {},
                       0u},
        ControlCommand{"discoverSystem",
                       "discover one system with the given name"
                       " and register the logging monitor for them",
                       &FepControl::discoverSystem,
                       {{"system name", &FepControl::noCompletion}},
                       0u},
        ControlCommand{"setCurrentWorkingDirectory",
                       "changes the current working dir of this fep_control instance",
                       &FepControl::setCurrentWorkingDirectory,
                       {{"directory name", &FepControl::noCompletion}},
                       0u},
        ControlCommand{"getCurrentWorkingDirectory",
                       "prints the current working dir of this fep_control instance",
                       &FepControl::getCurrentWorkingDirectory,
                       {},
                       0u},
        ControlCommand{"help",
                       "prints out the description of the commands",
                       &FepControl::help,
                       {{"command name", &FepControl::commandNameCompletion}},
                       1u},
        ControlCommand{"loadSystem",
                       "loads the given system",
                       &FepControl::loadSystem,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"unloadSystem",
                       "unloads the given system",
                       &FepControl::unloadSystem,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"setInitPriority",
                       "sets init priority of a participant",
                       &FepControl::setInitPriority,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion},
                        {"priority value", &FepControl::noCompletion}},
                       0u},
        ControlCommand{"getInitPriority",
                       "gets init priority of a participant",
                       &FepControl::getInitPriority,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"setStartPriority",
                       "sets start priority of a participant",
                       &FepControl::setStartPriority,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion},
                        {"priority value", &FepControl::noCompletion}},
                       0u},
        ControlCommand{"getStartPriority",
                       "gets start priority of a participant",
                       &FepControl::getStartPriority,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"initializeSystem",
                       "initializes the given system",
                       &FepControl::initializeSystem,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"deinitializeSystem",
                       "deinitializes the given system",
                       &FepControl::deinitializeSystem,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"startSystem",
                       "starts the given system",
                       &FepControl::startSystem,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"stopSystem",
                       "stops the given system",
                       &FepControl::stopSystem,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"pauseSystem",
                       "pauses the given system",
                       &FepControl::pauseSystem,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"shutdownSystem",
                       "shutdown the given system",
                       &FepControl::shutdownSystem,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"startMonitoringSystem",
                       "monitor logging messages of the given system",
                       &FepControl::startMonitoringSystem,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"stopMonitoringSystem",
                       "stop monitoring logging messages of the given system",
                       &FepControl::stopMonitoringSystem,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"loadParticipant",
                       "loads the given participant",
                       &FepControl::loadParticipant,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"unloadParticipant",
                       "unloads the given participant",
                       &FepControl::unloadParticipant,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"initializeParticipant",
                       "initializes the given participant",
                       &FepControl::initializeParticipant,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"deinitializeParticipant",
                       "deinitializes the given participant",
                       &FepControl::deinitializeParticipant,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"startParticipant",
                       "starts the given participant",
                       &FepControl::startParticipant,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"stopParticipant",
                       "stops the given participant",
                       &FepControl::stopParticipant,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"pauseParticipant",
                       "pauses the given participant",
                       &FepControl::pauseParticipant,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"getParticipantPropertyNames",
                       "display all property names of a participant",
                       &FepControl::getParticipantPropertyNames,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"getParticipantProperties",
                       "display all properties and values of a participant",
                       &FepControl::getParticipantProperties,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"getParticipantProperty",
                       "get value of a property of a participant",
                       &FepControl::getParticipantProperty,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion},
                        {"property_name", &FepControl::usedPropertiesCompletion}},
                       0u},
        ControlCommand{"setParticipantProperty",
                       "set value of a property of a participant",
                       &FepControl::setParticipantProperty,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion},
                        {"property_name", &FepControl::usedPropertiesCompletion},
                        {"property_value", &FepControl::noCompletion}},
                       0u},
        ControlCommand{"getParticipantRPCObjects",
                       "retrieve the RPC Objects of the given participant",
                       &FepControl::getRPCObjectsParticipant,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"getParticipantRPCObjectIIDs",
                       "retrieve the RPC IIDs of a concrete RPC Objects of the given participant",
                       &FepControl::getRPCObjectIIDSParticipant,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion},
                        {"object name", &FepControl::noCompletion}},
                       0u},
        ControlCommand{"getParticipantRPCObjectIIDDefinition",
                       "retrieve the RPC Definition of an IID of a concrete RPC Objects of the "
                       "given participant",
                       &FepControl::getRPCObjectDefinitionParticipant,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion},
                        {"object name", &FepControl::noCompletion},
                        {"interface id", &FepControl::noCompletion}},
                       0u},
        ControlCommand{"shutdownParticipant",
                       "shutdown the given participant",
                       &FepControl::shutdownParticipant,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"getSystemState",
                       "retrieves the given system",
                       &FepControl::getSystemState,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"setSystemState",
                       "sets the given system state",
                       &FepControl::setSystemState,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"system state", &FepControl::possibleSystemsStateCompletion}},
                       0u},
        ControlCommand{"getParticipantState",
                       "retrieves the given participants state",
                       &FepControl::getParticipantState,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"setParticipantState",
                       "sets the given participants system state",
                       &FepControl::setParticipantState,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"participant name", &FepControl::connectedParticipantsCompletion},
                        {"particiapnt state", &FepControl::possibleSystemsStateCompletion}},
                       0u},
        ControlCommand{"getParticipants",
                       "lists the participants of the given system",
                       &FepControl::getParticipants,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"configureTiming3SystemTime",
                       "configures the given system for timing"
                       " System Time (Sync only to the master)",
                       &FepControl::configureSystemTimingSystemTime,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"master participant name", &FepControl::connectedParticipantsCompletion}},
                       0u},
        ControlCommand{"configureTiming3DiscreteTime",
                       "configures the given system for timing"
                       " Discrete Time (for AFAP use 0.0 as factor)",
                       &FepControl::configureSystemTimingDiscrete,
                       {{"system name", &FepControl::connectedSystemsCompletion},
                        {"master participant name", &FepControl::connectedParticipantsCompletion},
                        {"factor", &FepControl::noCompletion},
                        {"step size (in ns)", &FepControl::noCompletion}},
                       0u},
        ControlCommand{"configureTiming3NoSync",
                       "resets the timing configuration",
                       &FepControl::configureSystemTimeNoSync,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"getCurrentTimingMaster",
                       "retrieves the timing master from the systems participants",
                       &FepControl::getCurrentTimingMaster,
                       {{"system name", &FepControl::connectedSystemsCompletion}},
                       0u},
        ControlCommand{"enableAutoDiscovery",
                       "enable the auto discovery for commands on systems",
                       &FepControl::enableAutoDiscovery,
                       {},
                       0u},
        ControlCommand{"disableAutoDiscovery",
                       "disable the auto discovery for commands on systems",
                       &FepControl::disableAutoDiscovery,
                       {},
                       0u},
        ControlCommand{"enableJson",
                       "enable json mode (hidden function)",
                       &FepControl::enableJsonMode,
                       {},
                       0u,
                       true},
        ControlCommand{"disableJson",
                       "disable json mode (hidden function)",
                       &FepControl::disableJsonMode,
                       {},
                       0u,
                       true}};
    return commands;
}
