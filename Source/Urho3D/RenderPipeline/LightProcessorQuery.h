// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Graphics/OctreeQuery.h"

#include <EASTL/vector.h>

namespace Urho3D
{

class DrawableProcessor;
class Light;

/// Result of light query for drawable.
struct LightGeometryQueryResult
{
    bool isLit_{};
    bool isForwardLit_{};
    bool isShadowCaster_{};
};

/// Frustum query for point light forward lit geometries and shadow casters.
class URHO3D_API PointLightGeometryQuery : public SphereOctreeQuery
{
public:
    PointLightGeometryQuery(ea::vector<Drawable*>& result, bool& hasLitGeometries, ea::vector<Drawable*>* shadowCasters,
        const DrawableProcessor* drawableProcessor, Light* light, unsigned primaryViewMask, unsigned shadowViewMask);

    void TestDrawables(Drawable** start, Drawable** end, bool inside) override;

private:
    LightGeometryQueryResult IsLitOrShadowCaster(Drawable* drawable, bool inside) const;

    /// Immutable
    /// @{
    const DrawableProcessor* drawableProcessor_{};
    const unsigned lightMask_{};
    const unsigned shadowViewMask_{};
    /// @}

    bool& hasLitGeometries_;
    /// Result array of shadow casters, if applicable.
    ea::vector<Drawable*>* shadowCasters_{};
};

/// Frustum query for spot light forward lit geometries and shadow casters.
class URHO3D_API SpotLightGeometryQuery : public FrustumOctreeQuery
{
public:
    SpotLightGeometryQuery(ea::vector<Drawable*>& result, bool& hasLitGeometries, ea::vector<Drawable*>* shadowCasters,
        const DrawableProcessor* drawableProcessor, Light* light, unsigned primaryViewMask, unsigned shadowViewMask);

    void TestDrawables(Drawable** start, Drawable** end, bool inside) override;

private:
    LightGeometryQueryResult IsLitOrShadowCaster(Drawable* drawable, bool inside) const;

    /// Immutable
    /// @{
    const DrawableProcessor* drawableProcessor_{};
    const unsigned lightMask_{};
    const unsigned shadowViewMask_{};
    /// @}

    bool& hasLitGeometries_;
    /// Result array of shadow casters, if applicable.
    ea::vector<Drawable*>* shadowCasters_{};
};

/// Frustum query for directional light shadow casters.
class URHO3D_API DirectionalLightShadowCasterQuery : public FrustumOctreeQuery
{
public:
    DirectionalLightShadowCasterQuery(ea::vector<Drawable*>& result,
        const Frustum& frustum, DrawableFlags drawableFlags, Light* light, unsigned viewMask);

    void TestDrawables(Drawable** start, Drawable** end, bool inside) override;

private:
    bool IsShadowCaster(Drawable* drawable, bool inside) const;

    const unsigned lightMask_{};
};

}
