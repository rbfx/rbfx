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


#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/ApplicationSettings.h>
#include <Urho3D/Engine/PluginApplication.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

#include "CacheRouter.h"

namespace Urho3D
{

class Player : public Application
{
public:
    ///
    explicit Player(Context* context);
    ///
    void Setup() override;
    ///
    void Start() override;
    ///
    void Stop() override;

protected:
    ///
    virtual bool LoadPlugins(const StringVector& plugins);
#if URHO3D_PLUGINS
    ///
    bool LoadAssembly(const ea::string& path);
    ///
    bool RegisterPlugin(PluginApplication* plugin);
#endif

    struct LoadedModule
    {
        SharedPtr<PluginModule> module_;
        SharedPtr<PluginApplication> application_;
    };

    ///
    ApplicationSettings settings_{context_};
    ///
    ea::vector<LoadedModule> plugins_;
    ///
    CacheRouter cacheRouter_{context_};
};

}
