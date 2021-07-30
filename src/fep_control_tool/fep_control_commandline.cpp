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

#include "fep_control_commandline.h"

#include "helper.h"
#include "linenoise_wrapper.h"

#include <a_util/filesystem.h>
#include <a_util/strings.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <mutex>

FepControlCommandLine::FepControlCommandLine(bool json_mode) : FepControl(json_mode)
{
}

std::vector<std::string> FepControlCommandLine::commandNameCompletion(
    const std::string& word_prefix)
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

std::vector<std::string> FepControlCommandLine::commandCompletion(const std::string& input)
{
    std::vector<std::string> input_tokens = parseLine(input);
    std::vector<std::string> completions;

    if (input_tokens.empty() || std::isspace(input.back())) {
        input_tokens.emplace_back();
    }

    if (input_tokens.size() == 1u) {
        return commandNameCompletion(input_tokens[0]);
    }
    else {
        auto it = findCommand(input_tokens[0]);
        if (it != getControlCommands().end()) {
            size_t index_in_args = input_tokens.size() - 2u;
            if (index_in_args < (*it)._arguments.size()) {
                auto completion_list = std::bind(
                    (*it)._arguments[index_in_args]._completion, this, input_tokens.back());
                auto exec = completion_list();
                if (!exec.empty()) {
                    input_tokens.pop_back();
                    std::string command_prefix = a_util::strings::join(input_tokens, " ") + " ";
                    for (const std::string& word_completion: exec) {
                        completions.push_back(command_prefix + word_completion);
                    }
                    return completions;
                }
            }
        }
    }

    return completions;
}

void FepControlCommandLine::printWelcomeMessage()
{
    std::lock_guard<std::mutex> lck(_mutex_write_output);
    std::cout << ("******************************************************************\n");
    std::cout << ("* Welcome to FEP Control(c) 2021 VW Group                        *\n");
    std::cout << ("*  use help to print help                                        *\n");
    std::cout << ("******************************************************************\n");
}

void FepControlCommandLine::readInputFromSource()
{
    printWelcomeMessage();

    line_noise::setCallback(
        std::bind(&FepControlCommandLine::commandCompletion, this, std::placeholders::_1));

    std::string line;
    while (line_noise::readLine(line)) {
        auto lineTokens = parseLine(line);
        if (lineTokens.empty()) {
            continue;
        }
        line_noise::addToHistory(line);
        processCommandline(lineTokens);
    }
}

void FepControlCommandLine::writeOutputToSink(const std::string& output)
{
    std::lock_guard<std::mutex> lck(_mutex_write_output);
    std::cout << output << std::flush;
}

void FepControlCommandLine::writeShutdownMessage()
{
    if (_json_mode) {
        writeOutputToSink("{\"action\" : \"applicationShutdown\"}");
    }
    else {
        writeOutputToSink("bye\n");
    }
}