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

#ifndef CONTROL_TOOL_CLIENT
#define CONTROL_TOOL_CLIENT
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace net = boost::asio;            // from <boost/asio.hpp>
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class ControlToolClient {
public:
    bool connectWebsocket();

    void sendMessage(const std::string& message);

    std::string receiveMessage();
    bool closeWebsocket();

private:
    std::string host = "127.0.0.1";
    const char* port = "9003";

    // The io_context is required for all I/O
    net::io_context ioc;
    // These objects perform our I/O
    tcp::resolver resolver{ioc};
    websocket::stream<tcp::socket> websocket_stream{ioc};
};
#endif //CONTROL_TOOL_CLIENT