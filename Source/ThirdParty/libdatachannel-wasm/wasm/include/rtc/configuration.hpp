/**
 * Copyright (c) 2017-2022 Paul-Louis Ageneau
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef RTC_CONFIGURATION_H
#define RTC_CONFIGURATION_H

#include "common.hpp"

#include <vector>

namespace rtc {

struct IceServer {
	enum class Type : int { Stun = 0, Turn, Dummy };
	enum class RelayType : int { TurnUdp = 0, TurnTcp, TurnTls };

	// Note: Contrary to libdatachannel, the URL constructor does not parse the URL.
	// Instead, it creates a Dummy IceServer to pass the URL as-is to the browser.
	IceServer(const string &url);

	// STUN
	IceServer(string hostname_, uint16_t port_);
	IceServer(string hostname_, string service_);

	// TURN
	IceServer(string hostname_, uint16_t port, string username_, string password_,
	          RelayType relayType_ = RelayType::TurnUdp);
	IceServer(string hostname_, string service_, string username_, string password_,
	          RelayType relayType_ = RelayType::TurnUdp);

	string hostname;
	uint16_t port;
	Type type;
	string username;
	string password;
	RelayType relayType;
};

struct Configuration {
	std::vector<IceServer> iceServers;
};

} // namespace rtc

#endif // RTC_CONFIGURATION_H
