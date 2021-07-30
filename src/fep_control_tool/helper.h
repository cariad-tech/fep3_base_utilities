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

#ifndef HELPER_H
#define HELPER_H

#include <fep_system/fep_system.h>
#include <json/json.h>
#include <string>

class CompactJsonStream {
public:
    CompactJsonStream()
    {
        _builder.settings_["indentation"] = "";
    }
    std::string convertJson(const Json::Value& json)
    {
        return Json::writeString(_builder, json);
    }

private:
    Json::StreamWriterBuilder _builder;
};

std::vector<std::string> parseLine(const std::string& line);

std::string resolveSystemState(const fep3::System::AggregatedState st);

fep3::SystemAggregatedState getStateFromString(const std::string& state_string);

#endif // HELPER_H
