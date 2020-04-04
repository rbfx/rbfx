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
#include <Urho3D/Scene/Node.h>

#include <Toolbox/SystemUI/AttributeInspector.h>

#include "Editor.h"
#include "Inspector/SerializableInspector.h"
#include "Tabs/InspectorTab.h"

namespace Urho3D
{

SerializableInspector::SerializableInspector(Context* context)
    : Object(context)
{
    auto* editor = GetSubsystem<Editor>();
    editor->onInspect_.Subscribe(this, &SerializableInspector::RenderInspector);
}

void SerializableInspector::RenderInspector(InspectArgs& args)
{
    auto* serializable = args.object_.Expired() ? nullptr : args.object_->Cast<Serializable>();
    if (serializable == nullptr || args.handledTimes_ > 0)
        return;

    args.handledTimes_++;
    ui::IdScope idScope(serializable);
    if (ui::CollapsingHeader(serializable->GetTypeName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        RenderAttributes(serializable, args.filter_, args.eventSender_);
}

}
