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


#include "fep_control_websocket.h"

#include "helper.h"

#include <boost/beast/core.hpp>
#include <iostream>
#include <mutex>
#include <thread>

FepControlWebsocket::FepControlWebsocket(boost::asio::ip::tcp::socket socket, bool json_mode)
    : FepControl(json_mode), _socket(std::move(socket))
{
}

void FepControlWebsocket::readInputFromSource()
{
    try {
        // Accept the websocket handshake
        _socket.accept();

        for (;;) {
            // This buffer will hold the incoming message
            boost::beast::multi_buffer buffer;

            // Read a message
            _socket.read(buffer);

            std::string input = boost::beast::buffers_to_string(buffer.data());
            // Log incoming message
            std::cout << "<-- " << input << std::endl;

            auto lineTokens = parseLine(input);
            if (lineTokens.empty()) {
                continue;
            }

            try
            {
                processCommandline(lineTokens);
            }
            catch (std::exception const& e)
            {
                writeError(lineTokens.front(), e.what(), CmdStatus::generic_error, "");
            }
        }
    }
    catch (boost::system::system_error const& se) {
        // Reaching this block indicates that the session was closed from client side.
        // Inform user about it and return from function. 
        if ((boost::asio::error::eof == se.code()) || (boost::asio::error::connection_reset == se.code()))
        {
            std::cout << "***Lost connection to client.***" << std::endl;
        }
        else if (boost::beast::websocket::error::closed == se.code())
        {
            std::cout << "***Client closed connection gracefully.***" << std::endl;
        }
        else if (boost::asio::error::operation_aborted == se.code())
        {
            std::cout << "***Lost connection to client. Read aborted***" << std::endl;
        }
        else
        {
            std::cout << "General Boost error reading from client: " << se.what() << std::endl;
        }
    }
}

void FepControlWebsocket::writeOutputToSink(const std::string& output)
{
    std::cout << "--> " << output << std::endl;
    try {
        std::lock_guard<std::mutex> lck(_mutex_write_output);
        _socket.write(boost::asio::buffer(output));
    }
    catch (const std::exception& ex)
    {
        std::cout << "***Cannot write to client.***" << std::endl;
        std::cout << ex.what() << std::endl;
        std::cout << " Tried to write: " << output << std::endl;
    }
}
void FepControlWebsocket::writeShutdownMessage()
{
    if (_json_mode) {
        writeOutputToSink("{\"action\" : \"applicationShutdown\"}");
    }
    else {
        writeOutputToSink("bye");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}