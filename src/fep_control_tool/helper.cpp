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

#include "helper.h"
#include "control_tool_common_helper.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace {

void skipWhitespaces(const char*& p, const char* additional_whitechars = nullptr)
{
    if (nullptr == p) {
        return;
    }
    if (additional_whitechars != nullptr) {
        while (std::isspace(*p) || (*p != '\0' && strchr(additional_whitechars, *p) != nullptr)) {
            p++;
        }
    }
    else {
        while (std::isspace(*p)) {
            p++;
        }
    }
}

bool getNextWord(const char*& src,
                 std::string& dest,
                 const char* additional_separator = nullptr,
                 const bool use_escape = true)
{
    if (nullptr == src) {
        return false;
    }
    dest.clear();

    skipWhitespaces(src);

    if (*src == '\0') {
        return false;
    }

    bool escape_active = false;
    char last_char = '\0';
    char quote = '\0';

    if (*src == '\"' || *src == '\'') {
        quote = *(src++);
        const char* src_start = src;

        while (*src != '\0' && (escape_active || *src != quote)) {
            escape_active = use_escape && (*src == '\\' && last_char != '\\'); // escape next char?
            last_char = *src;
            src++;
        }

        dest = std::string(src_start, src);

        if (*src == quote) {
            src++;
        }
    }
    else {
        const char* src_start = src;

        if (additional_separator == nullptr) {
            while (*src != '\0' && !std::isspace(*src)) {
                src++;
                if (*src == '\"' || *src == '\'') {
                    quote = *(src);

                    do {
                        escape_active = use_escape && (*src == '\\' && last_char != '\\');
                        last_char = *src;
                        src++;
                    } while (*src != '\0' && (escape_active || *src != quote));
                }
            }

            dest = std::string(src_start, src);
        }
        else {
            while (*src != '\0' &&
                   (!std::isspace(*src) && strchr(additional_separator, *src) == nullptr)) {
                src++;
            }

            dest = std::string(src_start, src);
        }
    }

    return true;
}

} // namespace

std::string resolveSystemState(const fep3::System::AggregatedState st)
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

std::vector<std::string> parseLine(const std::string& line, bool add_empty)
{
    const char* p = line.c_str();
    std::vector<std::string> words;
    std::string word;
    const char* last_p = p;
    while (getNextWord(p, word))
    {
        words.push_back(unEscape(word));
        last_p = p;
    }
    if (add_empty && last_p != p)
    {
        words.emplace_back();
    }
    return words;
}

fep3::SystemAggregatedState getStateFromString(const std::string& state_string)
{
    if (state_string == "shutdowned") {
        return fep3::SystemAggregatedState::unreachable;
    }
    else if (state_string == "unloaded") {
        return fep3::SystemAggregatedState::unloaded;
    }
    else if (state_string == "loaded") {
        return fep3::SystemAggregatedState::loaded;
    }
    else if (state_string == "initialized") {
        return fep3::SystemAggregatedState::initialized;
    }
    else if (state_string == "paused") {
        return fep3::SystemAggregatedState::paused;
    }
    else if (state_string == "running") {
        return fep3::SystemAggregatedState::running;
    }
    return fep3::SystemAggregatedState::undefined;
}
