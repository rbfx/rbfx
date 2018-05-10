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

namespace Urho3D
{

class PluginManager : public Object
{
    URHO3D_OBJECT(PluginManager, Object);
public:
    /// Construct.
    explicit PluginManager(Context* context) : Object(context) { }
    /// Load a plugin and return true if succeeded.
    virtual bool LoadPlugin(const String& path) = 0;
    /// Unload a plugin and return true if succeeded.
    virtual bool UnloadPlugin(const String& path) = 0;
    /// Load plugins from specified directory matching predetermined naming rules.
    void AutoLoadFrom(const String& directory);

protected:
    /// Returns true if specified path matches predefined plugin naming rules.
    virtual bool IsPluginPath(const String& path) = 0;
};

}
