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


#include <Urho3D/Engine/PluginApplication.h>
#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/Scene/Node.h>

namespace Urho3D
{

/// A custom component provided by the plugin.
class RotateObject
    : public LogicComponent
{
    URHO3D_OBJECT(RotateObject, LogicComponent
    );
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

    static void RegisterObject(Context* context, PluginApplication* plugin)
    {
        plugin->RegisterFactory<RotateObject>("User Components");
        URHO3D_ATTRIBUTE("Animate", bool, animate_, true, AM_EDIT);
    }

    bool animate_ = true;
};

}
