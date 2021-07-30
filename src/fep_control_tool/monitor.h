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


#ifndef MONITOR_H
#define MONITOR_H

#include <fep_system/fep_system.h>
#include "helper.h"

namespace Json {
class Value;
}

class FepControl;

class Monitor : public fep3::IEventMonitor {
public:
    Monitor(FepControl& parent, bool json_mode);
    void setJsonMode(const bool json_mode);

private:
    void onLog(std::chrono::milliseconds,
               fep3::LoggerSeverity severity_level,
               const std::string& participant_name,
               const std::string& logger_name,
               const std::string& message) override;

    bool _json_mode;
    FepControl& _parent;
    CompactJsonStream _builder;
};

#endif // MONITOR_H
