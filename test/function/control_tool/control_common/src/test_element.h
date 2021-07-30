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

#ifndef CONTROL_TOOL_TEST_ELEMENT_H
#define CONTROL_TOOL_TEST_ELEMENT_H

#include <fep3/core/participant_executor.hpp>
#include <fep3/core/element_base.h>

struct TestElement : public fep3::core::ElementBase 
{
    TestElement() : fep3::core::ElementBase("Testelement", "3.0")
    {
    }
};

struct PartStruct {
    PartStruct(PartStruct&&) = default;
    ~PartStruct() = default;
    PartStruct(fep3::core::Participant&& part) : _part(std::move(part)), _part_executor(_part)
    {
    }
    fep3::core::Participant _part;
    fep3::core::ParticipantExecutor _part_executor;
};
#endif //CONTROL_TOOL_TEST_ELEMENT_H
