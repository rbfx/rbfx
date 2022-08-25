//
// Copyright (c) 2022-2022 the rbfx project.
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
