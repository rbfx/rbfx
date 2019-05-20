//
// Copyright (c) 2017-2019 the rbfx project.
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


#include <regex>
#include "Converter.h"


namespace Urho3D
{

class GlobResources : public Converter
{
    URHO3D_OBJECT(GlobResources, Converter);
public:
    explicit GlobResources(Context* context);

    static void RegisterObject(Context* context);

    void Execute(const StringVector& input) override;

protected:
    void ConvertGlobToRegex();

    StringVector glob_;
    ea::vector<std::regex> regex_;
};

/// Return true if `string` matches any pattern specified in `patterns` list.
bool MatchesAny(const ea::string& string, const ea::vector<std::regex>& patterns);
/// Converts a glob expression to regex pattern. * matches anything except folder separators, ** matches anything
/// including folder separators.
std::regex GlobToRegex(const ea::string& expression);

}
