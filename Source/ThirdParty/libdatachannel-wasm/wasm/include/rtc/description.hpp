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

#ifndef RTC_DESCRIPTION_H
#define RTC_DESCRIPTION_H

#include "common.hpp"

#include <iostream>

namespace rtc {

class Description {
public:
	enum class Type { Unspec, Offer, Answer, Pranswer, Rollback };

	Description(const string &sdp, Type type);
	Description(const string &sdp, string typeString);

	Type type() const;
	string typeString() const;

	operator string() const;

	static Type stringToType(const string &typeString);
	static string typeToString(Type type);

private:
	string mSdp;
	string mType;
};

} // namespace rtc

std::ostream &operator<<(std::ostream &out, const rtc::Description &description);
std::ostream &operator<<(std::ostream &out, rtc::Description::Type type);

#endif // RTC_DESCRIPTION_H
