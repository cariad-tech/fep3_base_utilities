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


#ifndef FEP_CONTROL_COMMANDLINE_H
#define FEP_CONTROL_COMMANDLINE_H

#include "fep_control.h"

class FepControlCommandLine final : public FepControl {
public:
    explicit FepControlCommandLine(bool json_mode);

    void readInputFromSource();
    void writeOutputToSink(const std::string& output);
    void writeShutdownMessage();

private:
    std::vector<std::string> commandNameCompletion(const std::string& word_prefix);
    std::vector<std::string> commandCompletion(const std::string& input);
    void printWelcomeMessage();
};

#endif // FEP_CONTROL_COMMANDLINE_H