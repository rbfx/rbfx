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

#include "../Core/EditorPlugin.h"

#include <EASTL/vector.h>

namespace Urho3D
{

class EditorPluginManager : public Object
{
    URHO3D_OBJECT(EditorPluginManager, Object);

public:
    EditorPluginManager(Context* context);
    ~EditorPluginManager() override;

    /// Add new editor plugin. Should be called before any plugin user is initialized.
    void AddPlugin(SharedPtr<EditorPlugin> plugin);
    template <class T> void AddPlugin(const ea::string& name, EditorPluginFunction<T> function);
    /// Apply all plugins to the target.
    void Apply(Object* target);

    /// Return all plugins.
    const ea::vector<SharedPtr<EditorPlugin>>& GetPlugins() const { return plugins_; }

private:
    ea::vector<SharedPtr<EditorPlugin>> plugins_;
};

template <class T> void EditorPluginManager::AddPlugin(const ea::string& name, EditorPluginFunction<T> function)
{
    AddPlugin(MakeShared<EditorPluginT<T>>(context_, name, function));
}

}
