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

#include "../Precompiled.h"

#include "../SystemUI/DragDropPayload.h"

#include "../Scene/Component.h"
#include "../Scene/Scene.h"
#include "../SystemUI/SystemUI.h"

namespace Urho3D
{

void DragDropPayload::Set(const SharedPtr<DragDropPayload>& payload)
{
    if (auto context = Context::GetInstance())
        context->SetGlobalVar(DragDropPayloadVariable, MakeCustomValue(payload));
}

DragDropPayload* DragDropPayload::Get()
{
    if (const ImGuiPayload* payload = ui::GetDragDropPayload())
    {
        if (payload->DataType == DragDropPayloadType)
        {
            if (auto context = Context::GetInstance())
            {
                const Variant variant = context->GetGlobalVar(DragDropPayloadVariable);
                return dynamic_cast<DragDropPayload*>(variant.GetCustom<SharedPtr<DragDropPayload>>().Get());
            }
        }
    }
    return nullptr;
}

void DragDropPayload::UpdateSource(const CreateCallback& createPayload)
{
    ImGuiContext& g = *GImGui;

    ui::SetDragDropPayload(DragDropPayloadType.c_str(), nullptr, 0, ImGuiCond_Once);

    if (!g.DragDropPayload.Data)
    {
        const auto payload = createPayload();
        DragDropPayload::Set(payload);
        g.DragDropPayload.Data = payload;
    }

    const auto payload = DragDropPayload::Get();
    ui::TextUnformatted(payload->GetDisplayString().c_str());
}

void ResourceFileDescriptor::AddObjectType(const ea::string& typeName)
{
    types_.emplace(StringHash{typeName});
    typeNames_.emplace(typeName);
    mostDerivedType_ = typeName;
}

bool ResourceFileDescriptor::HasObjectType(const ea::string& typeName) const
{
    return typeNames_.contains(typeName);
}

bool ResourceFileDescriptor::HasObjectType(StringHash type) const
{
    return types_.contains(type);
}

bool ResourceFileDescriptor::HasExtension(ea::string_view extension) const
{
    return localName_.ends_with(extension, false);
}

bool ResourceFileDescriptor::HasExtension(std::initializer_list<ea::string_view> extensions) const
{
    return ea::any_of(extensions.begin(), extensions.end(),
        [this](ea::string_view extension) { return HasExtension(extension); });
}

ea::string ResourceDragDropPayload::GetDisplayString() const
{
    return resources_.size() == 1
        ? resources_[0].localName_
        : Format("{} items", resources_.size());
}

NodeComponentDragDropPayload::~NodeComponentDragDropPayload()
{
}

}
