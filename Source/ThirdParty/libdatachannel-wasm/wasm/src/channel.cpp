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

#include "channel.hpp"

namespace rtc {

using std::function;

size_t Channel::bufferedAmount() const { return 0; /* Dummy */ }

void Channel::onOpen(std::function<void()> callback) { mOpenCallback = std::move(callback); }

void Channel::onClosed(std::function<void()> callback) { mClosedCallback = std::move(callback); }

void Channel::onError(std::function<void(string)> callback) {
	mErrorCallback = std::move(callback);
}

void Channel::onMessage(std::function<void(message_variant data)> callback) {
	mMessageCallback = std::move(callback);
}

void Channel::onMessage(std::function<void(binary data)> binaryCallback,
                        std::function<void(string data)> stringCallback) {
	onMessage([binaryCallback = std::move(binaryCallback),
	           stringCallback = std::move(stringCallback)](message_variant data) {
		std::visit(overloaded{binaryCallback, stringCallback}, std::move(data));
	});
}

void Channel::onBufferedAmountLow(std::function<void()> callback) {
	mBufferedAmountLowCallback = std::move(callback);
}

void Channel::setBufferedAmountLowThreshold(size_t amount) { /* Dummy */
}

void Channel::triggerOpen() {
	if (mOpenCallback)
		mOpenCallback();
}

void Channel::triggerClosed() {
	if (mClosedCallback)
		mClosedCallback();
}

void Channel::triggerError(string error) {
	if (mErrorCallback)
		mErrorCallback(std::move(error));
}

void Channel::triggerMessage(const message_variant data) {
	if (mMessageCallback)
		mMessageCallback(data);
}

void Channel::triggerBufferedAmountLow() {
	if (mBufferedAmountLowCallback)
		mBufferedAmountLowCallback();
}

} // namespace rtc
