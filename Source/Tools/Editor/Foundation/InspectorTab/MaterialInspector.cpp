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

#include "../../Foundation/InspectorTab/MaterialInspector.h"

#include <Urho3D/Resource/ResourceCache.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

// TODO(editor): Extract to Widgets.h/cpp
#if 0
void ComboEx(const char* id, const StringVector& items, const ea::function<ea::optional<int>()>& getter, const ea::function<void(int)>& setter)
{
    const auto currentValue = getter();
    const char* currentValueLabel = currentValue && *currentValue < items.size() ? items[*currentValue].c_str() : "";
    if (!ui::BeginCombo(id, currentValueLabel))
        return;

    for (int index = 0; index < items.size(); ++index)
    {
        const char* label = items[index].c_str();
        if (ui::Selectable(label, currentValue && *currentValue == index))
            setter(index);
    }

    ui::EndCombo();
}

template <class ObjectType, class ElementType>
using GetterCallback = ea::function<ElementType(const ObjectType*)>;

template <class ObjectType, class ElementType>
using SetterCallback = ea::function<void(ObjectType*, const ElementType&)>;

template <class ObjectType, class ElementType>
GetterCallback<ObjectType, ElementType> MakeGetterCallback(ElementType (ObjectType::*getter)() const)
{
    return [getter](const ObjectType* object) { return (object->*getter)(); };
}

template <class ObjectType, class ElementType>
SetterCallback<ObjectType, ElementType> MakeSetterCallback(void (ObjectType::*setter)(ElementType))
{
    return [setter](ObjectType* object, const ElementType& value) { (object->*setter)(value); };
}

template <class ContainerType, class GetterCallbackType, class SetterCallbackType>
void ComboZip(const char* id, const StringVector& items, ContainerType& container, const GetterCallbackType& getter, const SetterCallbackType& setter)
{
    using ElementType = decltype(getter(container.front()));

    const auto wrappedGetter = [&]() -> ea::optional<int>
    {
        ea::optional<int> result;
        for (const auto& object : container)
        {
            const auto value = static_cast<int>(getter(object));
            if (!result)
                result = value;
            else if (*result != value)
                return ea::nullopt;
        }
        return result;
    };

    const auto wrappedSetter = [&](int value)
    {
        for (const auto& object : container)
            setter(object, static_cast<ElementType>(value));
    };

    ComboEx(id, items, wrappedGetter, wrappedSetter);
}
#endif

void Foundation_MaterialInspector(Context* context, InspectorTab_* inspectorTab)
{
    inspectorTab->RegisterAddon<MaterialInspector_>();
}

MaterialInspector_::MaterialInspector_(InspectorTab_* owner)
    : InspectorAddon(owner)
{
    auto project = owner_->GetProject();
    project->OnRequest.Subscribe(this, &MaterialInspector_::OnProjectRequest);
}

void MaterialInspector_::OnProjectRequest(ProjectRequest* request)
{
    auto inspectResourceRequest = dynamic_cast<InspectResourceRequest*>(request);
    if (!inspectResourceRequest || inspectResourceRequest->GetResources().empty())
        return;

    const auto& resources = inspectResourceRequest->GetResources();

    const bool areAllMaterials = ea::all_of(resources.begin(), resources.end(),
        [](const FileResourceDesc& desc) { return desc.GetTypeHint() == "material"; });
    if (!areAllMaterials)
        return;

    request->QueueProcessCallback([=]()
    {
        const auto resourceNames = inspectResourceRequest->GetSortedResourceNames();
        if (resourceNames_ != resourceNames)
        {
            resourceNames_ = resourceNames;
            InspectResources();
        }
    });
}

void MaterialInspector_::InspectResources()
{
    auto cache = GetSubsystem<ResourceCache>();

    MaterialInspectorWidget::MaterialVector materials;
    for (const ea::string& resourceName : resourceNames_)
    {
        if (auto material = cache->GetResource<Material>(resourceName))
            materials.emplace_back(material);
    }

    if (materials.empty())
    {
        widget_ = nullptr;
        return;
    }

    widget_ = MakeShared<MaterialInspectorWidget>(context_, materials);
    widget_->UpdateTechniques(techniquePath_);
    widget_->OnEditBegin.Subscribe(this, &MaterialInspector_::BeginEdit);
    widget_->OnEditEnd.Subscribe(this, &MaterialInspector_::EndEdit);

    Activate();
}

void MaterialInspector_::BeginEdit()
{
    URHO3D_ASSERT(!pendingAction_);

    auto project = owner_->GetProject();
    pendingAction_ = MakeShared<ModifyResourceAction>(project);
    for (Material* material : widget_->GetMaterials())
        pendingAction_->AddResource(material);
}

void MaterialInspector_::EndEdit()
{
    URHO3D_ASSERT(pendingAction_);

    auto project = owner_->GetProject();
    auto undoManager = project->GetUndoManager();

    undoManager->PushAction(pendingAction_);
    pendingAction_ = nullptr;

    for (Material* material : widget_->GetMaterials())
        project->SaveFileDelayed(material);
}

void MaterialInspector_::RenderContent()
{
    if (!widget_)
        return;

    if (updateTimer_.GetMSec(false) > updatePeriodMs_)
    {
        widget_->UpdateTechniques(techniquePath_);
        updateTimer_.Reset();
    }

    widget_->RenderTitle();
    ui::Separator();
    widget_->RenderContent();
}

void MaterialInspector_::RenderContextMenuItems()
{
}

void MaterialInspector_::RenderMenu()
{
}

void MaterialInspector_::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
}

}
