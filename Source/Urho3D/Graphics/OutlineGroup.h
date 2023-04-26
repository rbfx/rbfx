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

#include "../Graphics/Drawable.h"
#include "../Graphics/Material.h"
#include "../Scene/Component.h"
#include "../Scene/Node.h"

#include <EASTL/unordered_set.h>

namespace Urho3D
{

/// Group of selected drawables.
class URHO3D_API OutlineGroup : public Component
{
    URHO3D_OBJECT(OutlineGroup, Component);

public:
    OutlineGroup(Context* context);
    ~OutlineGroup() override;

    static void RegisterObject(Context* context);

    void ApplyAttributes() override;

    /// Attributes for serialization.
    /// @{
    void SetColor(const Color& color);
    const Color& GetColor() const { return color_; }
    void SetRenderOrder(unsigned renderOrder);
    unsigned GetRenderOrder() const { return renderOrder_; }
    void SetDebug(bool isDebug) { isDebug_ = isDebug; }
    bool IsDebug() const { return isDebug_; }
    void SetDrawablesAttr(const VariantVector& drawables);
    const VariantVector& GetDrawablesAttr() const;
    /// @}

    /// Return cached artificial material with only resources and shader parameters set.
    Material* GetOutlineMaterial(Material* referenceMaterial);

    /// Manage collection.
    /// @{
    bool HasDrawables() const { return !drawables_.empty(); }
    bool ContainsDrawable(Drawable* drawable) const { return drawables_.find_as(drawable) != drawables_.end(); }
    void ClearDrawables();
    /// Check if Drawable is present in group.
    bool HasDrawable(Drawable* drawable) const;
    /// Add drawable. Returns true if drawable added.
    bool AddDrawable(Drawable* drawable);
    /// Remove drawable. Returns true if drawable was removed.
    bool RemoveDrawable(Drawable* drawable);
    /// @}

private:
    struct MaterialKey
    {
        unsigned parametersHash_{};
        unsigned resourcesHash_{};

        MaterialKey() = default;
        explicit MaterialKey(const Material& material);

        bool operator==(const MaterialKey& rhs) const;
        unsigned ToHash() const;
    };

    Color color_{Color::WHITE};
    unsigned renderOrder_{DEFAULT_RENDER_ORDER};
    bool isDebug_{};

    /// Selected drawables.
    ea::unordered_set<WeakPtr<Drawable>> drawables_;
    bool drawablesDirty_{};
    mutable VariantVector drawablesAttr_;

    /// Cache of materials.
    ea::unordered_map<MaterialKey, SharedPtr<Material>> materials_;
};

}
