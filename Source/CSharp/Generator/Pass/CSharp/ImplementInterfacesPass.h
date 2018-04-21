//
// Copyright (c) 2008-2018 the Urho3D project.
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


#include <unordered_map>
#include "Pass/CppPass.h"

namespace std
{

template<> struct hash<std::weak_ptr<Urho3D::MetaEntity>>
{
    size_t operator()(std::weak_ptr<Urho3D::MetaEntity> const& ptr) const noexcept
    {
        return (size_t)ptr.lock().get();
    }
};

}

namespace Urho3D
{

/// Walk AST and mark suitable classes as interfaces. This has to be done beforehand so entity order does not cause
/// some interface methods be not implemented.
class DiscoverInterfacesPass : public CppApiPass
{
    public:
    explicit DiscoverInterfacesPass() { };
    bool Visit(MetaEntity* entity, cppast::visitor_info info) override;

    /// Map name of symbol to list of classes that multiple-inherit that symbol.
    std::unordered_map<std::string, std::vector<std::string>> inheritedBy_;
};

/// Copy methods into classes that implement interfaces but do not have these methods defined.
class ImplementInterfacesPass : public CppApiPass
{
    public:
    explicit ImplementInterfacesPass() { };
    bool Visit(MetaEntity* entity, cppast::visitor_info info) override;

};

}
