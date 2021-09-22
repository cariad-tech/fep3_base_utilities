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

#include "control_tool_client.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>

#include <future>
#include <iostream>

namespace http = beast::http;           // from <boost/beast/http.hpp>

bool ControlToolClient::connectWebsocket()
{
    // Set up websocket connection
    // Look up the domain name
    auto const results = resolver.resolve(host, port);
    bool connected = false;

    std::future<void> future = std::async(
        std::launch::async, []() { std::this_thread::sleep_for(std::chrono::seconds(3)); });
    std::future_status status = std::future_status::timeout;
    while (!connected && (status != std::future_status::ready)) {
        try {
            // Make the connection on the IP address we get from a lookup
            auto ep = net::connect(websocket_stream.next_layer(), results);

            // Update the host_ string. This will provide the value of the
            // Host HTTP header during the WebSocket handshake.
            // See https://tools.ietf.org/html/rfc7230#section-5.4
            host += ':' + std::to_string(ep.port());

            // Set a decorator to change the User-Agent of the handshake
            websocket_stream.set_option(
                websocket::stream_base::decorator([](websocket::request_type& req) {
                    req.set(http::field::user_agent,
                            std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-coro");
                }));

            // Perform the websocket handshake
            websocket_stream.handshake(host, "/api/v2/connect");
            connected = true;
        }
        catch (std::exception const& e) {
            std::cerr << "Socket not yet connected: " << e.what() << std::endl;
        }
        status = future.wait_for(std::chrono::milliseconds(10));
    }
    return connected;
}

void ControlToolClient::sendMessage(const std::string& message)
{
    websocket_stream.write(net::buffer(message));
}

std::string ControlToolClient::receiveMessage()
{
    beast::flat_buffer buffer;
    boost::system::error_code ec;
    websocket_stream.read(buffer, ec);
    std::cout << ec.message() << std::endl;
    return boost::beast::buffers_to_string(buffer.data());
}

bool ControlToolClient::closeWebsocket()
{
    boost::system::error_code ec;
    websocket_stream.close(websocket::close_code::normal, ec);
    std::cout << ec.message() << std::endl;
    return ec.value() == 0;
}