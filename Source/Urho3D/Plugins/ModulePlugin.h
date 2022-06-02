//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Plugins/DynamicModule.h"
#include "../Plugins/Plugin.h"

namespace Urho3D
{

/// Plugin that is loaded from a native or managed dynamic library.
class URHO3D_API ModulePlugin : public Plugin
{
    URHO3D_OBJECT(ModulePlugin, Plugin);

public:
    explicit ModulePlugin(Context* context) : Plugin(context) { }

    /// Implement Plugin
    /// @{
    bool Load() override;
    bool IsLoaded() const override;
    bool IsOutOfDate() const override;
    bool WaitForCompleteFile(unsigned timeoutMs) const override;
    bool PerformUnload() override;
    /// @}

protected:
    /// Converts name to a full plugin file path. Returns empty string on error.
    ea::string NameToPath(const ea::string& name) const;
    /// Return path to the current version of the module.
    ea::string GetVersionModulePath(const ea::string& path);

    /// Unversioned plugin module path.
    ea::string path_;
    /// Native module of this plugin.
    DynamicModule module_{context_};
    /// Last modification time.
    unsigned mtime_{};
    /// Last loaded module type.
    ModuleType lastModuleType_{};
};


}
