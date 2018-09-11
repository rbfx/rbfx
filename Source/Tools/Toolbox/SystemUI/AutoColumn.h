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


#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include "ToolboxAPI.h"


namespace Urho3D
{

class URHO3D_TOOLBOX_API AutoColumn : public Object
{
    URHO3D_OBJECT(AutoColumn, Object);
public:
    /// Construct.
    explicit AutoColumn(Context* context) : Object(context)
    {
        SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&) {
            lastMaxWidth_ = currentMaxWidth_;
            currentMaxWidth_ = 0;
        });
    }

    /// Automatically creates two columns where first column is as wide as longest label.
    void NextColumn()
    {
        ui::SameLine();
        currentMaxWidth_ = Max(currentMaxWidth_, ui::GetCursorPosX());
        ui::SameLine(lastMaxWidth_);
    }

protected:
    /// Max width of attribute label.
    int lastMaxWidth_ = 0;
    /// Max width of attribute label.
    int currentMaxWidth_ = 0;
};

}
