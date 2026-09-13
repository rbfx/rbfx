// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once


#include <Urho3D/Plugins/PluginApplication.h>
#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/Scene/Node.h>

namespace Urho3D
{

/// A custom component provided by the plugin.
class RotateObject
    : public LogicComponent
{
    URHO3D_OBJECT(RotateObject, LogicComponent);

public:
    RotateObject(Context* context)
        : LogicComponent(context)
    {
        SetUpdateEventMask(USE_UPDATE);
    }

    void Update(float timeStep) override
    {
        if (animate_)
            GetNode()->Rotate(Quaternion(10 * timeStep, 20 * timeStep, 30 * timeStep));
    }

    static void RegisterObject(Context* context)
    {
        context->AddFactoryReflection<RotateObject>(Category_User);
        URHO3D_ATTRIBUTE("Animate", bool, animate_, true, AM_EDIT);
    }

    bool animate_ = true;
};

}
