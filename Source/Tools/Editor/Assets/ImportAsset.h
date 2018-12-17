//
// Copyright (c) 2018 Rokas Kupstys
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


#include <Urho3D/Core/Object.h>
#include <Toolbox/IO/ContentUtilities.h>


namespace Urho3D
{

class ImportAsset : public Object
{
    URHO3D_OBJECT(ImportAsset, Object);
public:
    /// Construct.
    explicit ImportAsset(Context* context);

    /// Returns true if path points to a resource that can be converted by this converter.
    virtual bool Accepts(const String& path, ContentType type) = 0;
    /// A conversion function. Default implementation starts editor as subprocess to convert the asset.
    virtual bool Convert(const String& path);
    /// A function that does conversion. If it can be executed in a worker thread you may override Convert() and call RunConvert() from it directly.
    virtual bool RunConverter(const String& path) = 0;
};

}
