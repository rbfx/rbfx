// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Core/Object.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>

namespace Urho3D
{

/// Base class for any Editor plugin.
class EditorPlugin : public Object
{
    URHO3D_OBJECT(EditorPlugin, Object);

public:
    using Object::Object;

    virtual const ea::string& GetName() = 0;
    virtual bool Apply(Object* target) = 0;
};

template <class T>
using EditorPluginFunction = void(*)(Context* context, T* target);

template <class T>
class EditorPluginT : public EditorPlugin
{
public:
    EditorPluginT(Context* context, const ea::string& name, EditorPluginFunction<T> function)
        : EditorPlugin(context)
        , name_(name)
        , function_(function)
    {
    }

    const ea::string& GetName() final { return name_; }

    bool Apply(Object* target) final
    {
        if (auto derivedTarget = dynamic_cast<T*>(target))
        {
            function_(context_, derivedTarget);
            return true;
        }
        return false;
    }

private:
    ea::string name_;
    EditorPluginFunction<T> function_;
};

}
