// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
