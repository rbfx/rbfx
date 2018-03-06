//
// Copyright (c) 2008-2017 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

/// Implement operators for using enum members as flags
#define URHO3D_TO_FLAGS_ENUM(t) \
    inline t operator|(t a, t b) { return static_cast<t>(static_cast<size_t>(a) | static_cast<size_t>(b)); }\
    inline t operator|=(t& a, t b) { a = static_cast<t>(static_cast<size_t>(a) | static_cast<size_t>(b)); return a; }\
    inline t operator&(t a, t b) { return static_cast<t>(static_cast<size_t>(a) & static_cast<size_t>(b)); }\
    inline t operator&=(t& a, t b) { a = static_cast<t>(static_cast<size_t>(a) & static_cast<size_t>(b)); return a; }\
    inline t operator^(t a, t b) { return static_cast<t>(static_cast<size_t>(a) ^ static_cast<size_t>(b)); }\
    inline t operator^=(t& a, t b) { a = static_cast<t>(static_cast<size_t>(a) ^ static_cast<size_t>(b)); return a; }\
    inline t operator~(t a) { return static_cast<t>(~static_cast<size_t>(a)); }

#if __cplusplus >= 201703L
#   define URHO3D_FALLTHROUGH [[fallthrough]]
#else
#   define URHO3D_FALLTHROUGH
#endif
