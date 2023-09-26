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

#include "datachannel.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

#include <exception>
#include <stdexcept>

extern "C" {
extern void rtcDeleteDataChannel(int dc);
extern int rtcGetDataChannelLabel(int dc, char *buffer, int size);
extern void rtcSetOpenCallback(int dc, void (*openCallback)(void *));
extern void rtcSetErrorCallback(int dc, void (*errorCallback)(const char *, void *));
extern void rtcSetMessageCallback(int dc, void (*messageCallback)(const char *, int, void *));
extern void rtcSetBufferedAmountLowCallback(int dc, void (*bufferedAmountLowCallback)(void *));
extern int rtcGetBufferedAmount(int dc);
extern void rtcSetBufferedAmountLowThreshold(int dc, int threshold);
extern int rtcSendMessage(int dc, const char *buffer, int size);
extern void rtcSetUserPointer(int i, void *ptr);
extern bool getDataChannelOrdered(int dc);
extern int getDataChannelMaxPacketLifeTime(int dc);
extern int getDataChannelMaxRetransmits(int dc);
}

namespace rtc {

using std::function;

void DataChannel::OpenCallback(void *ptr) {
	DataChannel *d = static_cast<DataChannel *>(ptr);
	if (d)
		d->triggerOpen();
}

void DataChannel::ErrorCallback(const char *error, void *ptr) {
	DataChannel *d = static_cast<DataChannel *>(ptr);
	if (d)
		d->triggerError(string(error ? error : "unknown"));
}

void DataChannel::MessageCallback(const char *data, int size, void *ptr) {
	DataChannel *d = static_cast<DataChannel *>(ptr);
	if (d) {
		if (data) {
			if (size >= 0) {
				auto *b = reinterpret_cast<const byte *>(data);
				d->triggerMessage(binary(b, b + size));
			} else {
				d->triggerMessage(string(data));
			}
		} else {
			d->close();
			d->triggerClosed();
		}
	}
}

void DataChannel::BufferedAmountLowCallback(void *ptr) {
	DataChannel *d = static_cast<DataChannel *>(ptr);
	if (d) {
		d->triggerBufferedAmountLow();
	}
}

DataChannel::DataChannel(int id) : mId(id), mConnected(false) {
	rtcSetUserPointer(mId, this);
	rtcSetOpenCallback(mId, OpenCallback);
	rtcSetErrorCallback(mId, ErrorCallback);
	rtcSetMessageCallback(mId, MessageCallback);
	rtcSetBufferedAmountLowCallback(mId, BufferedAmountLowCallback);

	char str[256];
	rtcGetDataChannelLabel(mId, str, 256);
	mLabel = str;
}

DataChannel::~DataChannel() { close(); }

void DataChannel::close() {
	mConnected = false;
	if (mId) {
		rtcDeleteDataChannel(mId);
		mId = 0;
	}
}

bool DataChannel::send(message_variant message) {
	if (!mId)
		return false;

	return std::visit(
	    overloaded{[this](const binary &b) {
		               auto data = reinterpret_cast<const char *>(b.data());
		               return rtcSendMessage(mId, data, int(b.size())) >= 0;
	               },
	               [this](const string &s) { return rtcSendMessage(mId, s.c_str(), -1) >= 0; }},
	    std::move(message));
}

bool DataChannel::send(const byte *data, size_t size) {
	if (!mId)
		return false;

	return rtcSendMessage(mId, reinterpret_cast<const char *>(data), int(size)) >= 0;
}

bool DataChannel::isOpen() const { return mConnected; }

bool DataChannel::isClosed() const { return mId == 0; }

size_t DataChannel::bufferedAmount() const {
	if (!mId)
		return 0;

	int ret = rtcGetBufferedAmount(mId);
	if (ret < 0)
		return 0;

	return size_t(ret);
}

std::string DataChannel::label() const { return mLabel; }

Reliability DataChannel::reliability() const {
    Reliability reliability;
    using namespace emscripten;
    using namespace std::chrono_literals;
    bool ordered = getDataChannelOrdered(mId);
    int maxPacketLifeTime = getDataChannelMaxPacketLifeTime(mId);
    int maxRetransmits = getDataChannelMaxRetransmits(mId);
    reliability.unordered = !ordered;
    if (maxPacketLifeTime >= 0) {
        reliability.type = Reliability::Type::Timed;
        reliability.rexmit = 1ms * maxPacketLifeTime;
    }
    else if (maxRetransmits >= 0) {
        reliability.type = Reliability::Type::Rexmit;
        reliability.rexmit = maxRetransmits;
    }
    return reliability;
}

void DataChannel::setBufferedAmountLowThreshold(size_t amount) {
	if (!mId)
		return;

	rtcSetBufferedAmountLowThreshold(mId, int(amount));
}

void DataChannel::triggerOpen() {
	mConnected = true;
	Channel::triggerOpen();
}

} // namespace rtc
