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

#ifndef CONTROL_TOOL_TEST_HELPERS_H
#define CONTROL_TOOL_TEST_HELPERS_H

#include <string>

#define STR(x) #x
#define STRINGIZE(x) STR(x)
namespace {
const std::string binary_tool_path = STRINGIZE(BINARY_TOOL_PATH);
}
#undef STR
#undef STRINGIZE

#endif
