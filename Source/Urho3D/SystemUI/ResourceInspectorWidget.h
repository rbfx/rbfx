// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Signal.h"
#include "../Graphics/Animation.h"
#include "../SystemUI/BaseWidget.h"
#include "../SystemUI/Widgets.h"

namespace Urho3D
{

/// SystemUI widget used to edit resources.
class URHO3D_API ResourceInspectorWidget : public BaseWidget
{
    URHO3D_OBJECT(ResourceInspectorWidget, BaseWidget);

public:
    struct PropertyDesc
    {
        ea::string name_;
        Variant defaultValue_;
        ea::function<Variant(const Resource* material)> getter_;
        ea::function<void(Resource* material, const Variant& value)> setter_;
        ea::string hint_;
        Widgets::EditVariantOptions options_;
    };

    Signal<void()> OnEditBegin;
    Signal<void()> OnEditEnd;

    using ResourceVector = ea::vector<SharedPtr<Resource>>;

    ResourceInspectorWidget(Context* context, const ResourceVector& resources, ea::span<const PropertyDesc> properties);
    ~ResourceInspectorWidget() override;

    void RenderTitle();
    void RenderContent() override;

    virtual bool CanSave() const { return true; }
    const ResourceVector& GetResources() const { return resources_; }

private:
    const ea::span<const PropertyDesc> properties;

    void RenderProperties(const PropertyDesc& desc);
    void RenderProperty(const PropertyDesc& desc);

    ea::vector<ea::pair<const PropertyDesc*, Variant>> pendingSetProperties_;

    ResourceVector resources_;
};

} // namespace Urho3D
