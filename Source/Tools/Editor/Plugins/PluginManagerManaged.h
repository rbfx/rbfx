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


#include "PluginManager.h"


namespace Urho3D
{

class PluginManagerManaged : public PluginManager
{
    URHO3D_OBJECT(PluginManagerManaged, PluginManager);
public:
    /// Construct.
    explicit PluginManagerManaged(Context* context);
    /// Load a plugin and return true if succeeded.
    bool LoadPlugin(const String& path) override;
    /// Unload a plugin and return true if succeeded.
    bool UnloadPlugin(const String& path) override;

protected:
    /// Handles reloading of plugins.
    void OnEndFrame();
    /// Returns true if specified path matches predefined plugin naming rules.
    bool IsPluginPath(const String& path) override;

    /// Map plugin path to reloadable context.
    HashMap<String, gchandle> plugins_;
};

}
