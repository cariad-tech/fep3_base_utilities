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


#include "monitor.h"

#include "fep_control.h"
#include "helper.h"

#include <json/json.h>
#include <sstream>

Monitor::Monitor(FepControl& parent, bool json_mode) : _parent(parent), _json_mode(json_mode)
{
}

void Monitor::setJsonMode(const bool json_mode)
{
    _json_mode = json_mode;
}

void Monitor::onLog(std::chrono::milliseconds,
                    fep3::LoggerSeverity severity_level,
                    const std::string& participant_name,
                    const std::string& logger_name, // depends on the Category ...
                    const std::string& message)
{
    if (_json_mode) {
        Json::Value log;
        log["log_type"] = "message";
        log["severity_level"] = getString(severity_level);
        log["participant_name"] = participant_name;
        log["logger_name"] = logger_name;
        log["message"] = message;
        _parent.writeOutput(_builder.convertJson(log), "\n");
    }
    else {
        // clang-format off
        _parent.writeOutput("    LOG [", getString(severity_level), "] ",
                            logger_name, "@", participant_name, " :", message, "\n", "fep> ");
        // clang-format on
    }
}
