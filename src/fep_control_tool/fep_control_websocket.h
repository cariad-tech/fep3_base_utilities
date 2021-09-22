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


#ifndef FEP_CONTROL_WEBSOCKET_H
#define FEP_CONTROL_WEBSOCKET_H

#include "fep_control.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/websocket.hpp>

class FepControlWebsocket final : public FepControl {
public:
    FepControlWebsocket(boost::asio::ip::tcp::socket socket, bool json_mode);

    void readInputFromSource();
    void writeOutputToSink(const std::string& output);
    void writeShutdownMessage();

private:
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> _socket;
};

#endif // FEP_CONTROL_WEBSOCKET_H