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

#include "description.hpp"

#include <unordered_map>

namespace rtc {

Description::Description(const string &sdp, Type type) : mSdp(sdp), mType(typeToString(type)) {}

Description::Description(const string &sdp, string typeString)
    : mSdp(sdp), mType(std::move(typeString)) {}

Description::Type Description::type() const { return stringToType(mType); }

string Description::typeString() const { return mType; }

Description::operator string() const { return mSdp; }

Description::Type Description::stringToType(const string &typeString) {
	using TypeMap_t = std::unordered_map<string, Type>;
	static const TypeMap_t TypeMap = {{"unspec", Type::Unspec},
	                                  {"offer", Type::Offer},
	                                  {"answer", Type::Answer},
	                                  {"pranswer", Type::Pranswer},
	                                  {"rollback", Type::Rollback}};
	auto it = TypeMap.find(typeString);
	return it != TypeMap.end() ? it->second : Type::Unspec;
}

string Description::typeToString(Type type) {
	switch (type) {
	case Type::Unspec:
		return "unspec";
	case Type::Offer:
		return "offer";
	case Type::Answer:
		return "answer";
	case Type::Pranswer:
		return "pranswer";
	case Type::Rollback:
		return "rollback";
	default:
		return "unknown";
	}
}

} // namespace rtc

std::ostream &operator<<(std::ostream &out, const rtc::Description &description) {
	return out << std::string(description);
}

std::ostream &operator<<(std::ostream &out, rtc::Description::Type type) {
	return out << rtc::Description::typeToString(type);
}
