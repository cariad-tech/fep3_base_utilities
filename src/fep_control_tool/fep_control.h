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


#ifndef FEP_CONTROL_H
#define FEP_CONTROL_H
#include "monitor.h"

#include <functional>
#include <map>
#include <mutex>
#include <sstream>
#include <vector>
#include <boost/optional.hpp>

class FepControl;

typedef std::vector<std::string>::const_iterator TokenIterator;
typedef std::pair<std::string, std::string> Attribute;
typedef std::vector<Attribute> Attributes;
typedef std::vector<Attributes> AttributesVec;

using ActionFunction = bool (FepControl::*)(TokenIterator first, TokenIterator last);
using ArgumentCompletionFunction =
    std::vector<std::string> (FepControl::*)(const std::string& input);

struct ArgumentHandler {
    std::string _description;
    ArgumentCompletionFunction _completion;
};

struct ControlCommand {
    std::string _name, _description;
    ActionFunction _action;
    std::vector<ArgumentHandler> _arguments;
    size_t _last_optional_parameters;
    bool _hidden = false;
};

enum class CmdStatus : std::uint8_t {
    no_error = 0,
    generic_error = 1,
    input_error = 2,
    filesystem_error = 3,
    statechange_error = 4,
    systemconfig_error = 5,
    systemtiming_error = 6,
    participant_error = 7,
    rpcobject_error = 8
};

enum class PriorityType : std::uint8_t {
    init_priority = 0,
    start_priority = 1,
};

class FepControl {
public:
    explicit FepControl(bool json_mode);

    FepControl(const FepControl&) = default;
    FepControl& operator=(const FepControl&) = default;
    FepControl(FepControl&&) = default;
    FepControl& operator=(FepControl&&) = default;

    int processCommandline(const std::vector<std::string>& command_line);
    virtual void readInputFromSource() = 0;
    virtual void writeShutdownMessage() = 0;
    const std::vector<ControlCommand>& getControlCommands() const noexcept;

    template <typename... Args>
    std::string writeOutput(Args&&... args)
    {
        // concatenate the args to string, then send it to the user
        std::ostringstream stream;
        using List = int[];
        (void)List{0, (void(stream << std::forward<Args>(args)), 0)...};

        const std::string output = stream.str();

        writeOutputToSink(output);

        return output;
    }

protected:
    ~FepControl() = default;
    std::vector<ControlCommand>::const_iterator findCommand(const std::string& command_candidate);
    void writeError(const std::string& action,
                    const std::string& error,
                    const CmdStatus status,
                    const std::string& reason);
    bool _json_mode = false;
    std::mutex _mutex_write_output;
    CompactJsonStream _builder;

private:
    // pure virtual methods. Must be specified in derived classes
    virtual void writeOutputToSink(const std::string& output) = 0;
    // helper and ordinary methods
    void discoverSystemByName(const std::string& name);
    std::map<std::string, fep3::System>::iterator getConnectedOrDiscoveredSystem(
        const std::string& name, const bool auto_discovery, const std::string& action);
    std::vector<std::string> usedPropertiesCompletion(const std::string& word_prefix);
    std::vector<std::string> noCompletion(const std::string&);
    std::vector<std::string> localFilesCompletion(const std::string& word_prefix);
    std::vector<std::string> connectedSystemsCompletion(const std::string& word_prefix);
    std::vector<std::string> connectedParticipantsCompletion(const std::string& word_prefix);

    void writeNote(const std::string& action, const std::string& note);
    void writeNote(const std::string& action, const Attribute& attribute);
    void writeNotes(const std::string& action, const Attributes& attributes);
    void writeNotes(const std::string& action, const AttributesVec& attributes_vec);

    void writeException(const std::string& action,
                        const std::string& exception,
                        const CmdStatus status,
                        const std::exception& e);

    Attributes getSystemParticipants(const fep3::System& system);
    AttributesVec getSystems(const std::vector<fep3::System>& systems);

    boost::optional<fep3::ParticipantProxy> getParticipant(const std::string& action, 
                                                           const std::string& system_name, 
                                                           const std::string& participant_name);
    void buildRPCRequest(const std::string& request_name, 
                         const std::string& request_arguments,
                         std::string& result);

    void parseJsonString(const std::string& json_string,
                         Json::Value& result);
    void sendRPCRequest(fep3::ParticipantProxy participant);


    // corresponding methods to user commands
    bool discoverAllSystems(TokenIterator first, TokenIterator);
    bool discoverSystem(TokenIterator first, TokenIterator);
    bool setCurrentWorkingDirectory(TokenIterator first, TokenIterator);
    bool getCurrentWorkingDirectory(TokenIterator, TokenIterator);
    bool connectSystem(TokenIterator first, TokenIterator);
    bool setPriority(TokenIterator first, TokenIterator, const PriorityType type);
    bool getPriority(TokenIterator first, TokenIterator, const PriorityType type);
    bool setInitPriority(TokenIterator first, TokenIterator);
    bool getInitPriority(TokenIterator first, TokenIterator);
    bool setStartPriority(TokenIterator first, TokenIterator);
    bool getStartPriority(TokenIterator first, TokenIterator);
    bool changeStateMethod(TokenIterator first,
                           std::function<void(fep3::System& system)> call,
                           const std::string& action,
                           const std::string& success_message,
                           const std::string& failed_message);
    bool startSystem(TokenIterator first, TokenIterator);
    bool stopSystem(TokenIterator first, TokenIterator);
    bool loadSystem(TokenIterator first, TokenIterator);
    bool unloadSystem(TokenIterator first, TokenIterator);
    bool initializeSystem(TokenIterator first, TokenIterator);
    bool deinitializeSystem(TokenIterator first, TokenIterator);
    bool pauseSystem(TokenIterator first, TokenIterator);
    bool shutdownSystem(TokenIterator first, TokenIterator);
    bool startMonitoringSystem(TokenIterator first, TokenIterator);
    bool stopMonitoringSystem(TokenIterator first, TokenIterator);
    bool doParticipantStateChange(
        TokenIterator& first,
        std::function<void(fep3::RPCComponent<fep3::rpc::IRPCParticipantStateMachine>&)>
            change_state,
        const std::string& action,
        const std::string& message_1,
        const std::string& message_2);
    bool loadParticipant(TokenIterator first, TokenIterator);
    bool unloadParticipant(TokenIterator first, TokenIterator);
    bool initializeParticipant(TokenIterator first, TokenIterator);
    bool deinitializeParticipant(TokenIterator first, TokenIterator);
    bool startParticipant(TokenIterator first, TokenIterator);
    bool stopParticipant(TokenIterator first, TokenIterator);
    bool pauseParticipant(TokenIterator first, TokenIterator);
    bool shutdownParticipant(TokenIterator first, TokenIterator);
    bool getSystemState(TokenIterator first, TokenIterator);
    bool setSystemState(TokenIterator first, TokenIterator);
    bool getParticipants(TokenIterator first, TokenIterator);
    bool configureSystem(TokenIterator first, TokenIterator);
    bool quit(TokenIterator first, TokenIterator);
    bool configureSystemTimingSystemTime(TokenIterator first, TokenIterator);
    bool configureSystemTimingDiscrete(TokenIterator first, TokenIterator);
    bool configureSystemTimeNoSync(TokenIterator first, TokenIterator);
    bool getCurrentTimingMaster(TokenIterator first, TokenIterator);
    bool enableAutoDiscovery(TokenIterator first, TokenIterator);
    bool disableAutoDiscovery(TokenIterator first, TokenIterator);
    bool enableJsonMode(TokenIterator first, TokenIterator);
    bool disableJsonMode(TokenIterator first, TokenIterator);
    bool getParticipantState(TokenIterator first, TokenIterator);
    bool setParticipantState(TokenIterator first, TokenIterator);
    bool getParticipantPropertyNames(TokenIterator first, TokenIterator);
    bool getParticipantProperties(TokenIterator first, TokenIterator);
    bool getParticipantProperty(TokenIterator first, TokenIterator);
    bool setParticipantProperty(TokenIterator first, TokenIterator);
    bool getRPCObjectsParticipant(TokenIterator first, TokenIterator);
    bool getRPCObjectIIDSParticipant(TokenIterator first, TokenIterator);
    bool getRPCObjectDefinitionParticipant(TokenIterator first, TokenIterator);
    bool callRPC(TokenIterator first, TokenIterator last);
    bool help(TokenIterator first, TokenIterator last);
    std::vector<std::string> commandNameCompletion(const std::string& word_prefix);
    std::vector<std::string> possibleSystemsStateCompletion(const std::string& word_prefix);

    // private member
    std::map<std::string, fep3::System> _connected_or_discovered_systems;
    bool _auto_discovery_of_systems = false;
    std::string _last_system_name_used = "";
    const std::string _empty_system_name = "-";
    std::vector<std::string> _used_properties = {
        "clock/main_clock", "clock/step_size", "clock/time_factor"};
    Monitor monitor;
};

#endif // FEP_CONTROL_H
