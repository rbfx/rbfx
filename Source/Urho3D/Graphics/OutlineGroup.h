// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/Drawable.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Scene/Component.h"
#include "Urho3D/Scene/Node.h"

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
