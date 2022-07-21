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
#pragma once
#include <algorithm>
#include <cassert>
#include <cctype>
#include <string>

inline std::string quoteNameIfNecessary(const std::string& name)
{
    bool need_quotes = name.empty() || std::find_if(name.begin(), name.end(), isspace) != name.end();
    bool need_backslash = std::find_if(name.begin(), name.end(), [](char c) { return c == '"' || c == '\\'; }) != name.end();
    if (!need_quotes && !need_backslash)
    {
        return name;
    }
    std::string quoted_name = need_quotes ? "\"" : "";
    for (auto c : name)
    {
        if (c == '"' || c == '\\')
        {
            quoted_name += '\\';
        }
        quoted_name += c;
    }
    if (need_quotes)
    {
        quoted_name += '"';
    }
    return quoted_name;
}

inline std::string unEscape(const std::string& name)
{
    std::string unescaped;
    bool escape_active = false;
    for (char c : name)
    {
        if (escape_active || c != '\\')
        {
            unescaped += c;
            escape_active = false;
        }
        else // !escape_activ && ec == '\\'
        {
            escape_active = true;
        }
    }
    if (escape_active)
    {
        unescaped += '\\';
    }
    return unescaped;
}
