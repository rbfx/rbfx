// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Core/IteratorRange.h"
#include "../Graphics/Light.h"
#include "../RenderPipeline/DrawableProcessor.h"
#include "../RenderPipeline/LightProcessorQuery.h"
#include "../Scene/Node.h"

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

/// Return light sphere for the query.
Sphere GetLightSphere(Light* light)
{
    return Sphere(light->GetNode()->GetWorldPosition(), light->GetRange());
}

}

PointLightGeometryQuery::PointLightGeometryQuery(
    ea::vector<Drawable*>& result, bool& hasLitGeometries, ea::vector<Drawable*>* shadowCasters,
    const DrawableProcessor* drawableProcessor, Light* light, unsigned primaryViewMask, unsigned shadowViewMask)
    : SphereOctreeQuery(result, GetLightSphere(light), DRAWABLE_GEOMETRY, primaryViewMask | shadowViewMask)
    , hasLitGeometries_(hasLitGeometries)
    , shadowCasters_(shadowCasters)
    , drawableProcessor_(drawableProcessor)
    , lightMask_(light->GetLightMaskEffective())
    , shadowViewMask_(shadowViewMask)
{
    hasLitGeometries_ = false;
    if (shadowCasters_)
        shadowCasters_->clear();
}

void PointLightGeometryQuery::TestDrawables(Drawable** start, Drawable** end, bool inside)
{
    for (Drawable* drawable : MakeIteratorRange(start, end))
    {
        const auto result = IsLitOrShadowCaster(drawable, inside);
        if (result.isLit_)
            hasLitGeometries_ = true;
        if (result.isForwardLit_)
            result_.push_back(drawable);
        if (result.isShadowCaster_)
            shadowCasters_->push_back(drawable);
    }
}

LightGeometryQueryResult PointLightGeometryQuery::IsLitOrShadowCaster(Drawable* drawable, bool inside) const
{
    const unsigned drawableIndex = drawable->GetDrawableIndex();
    const unsigned geometryFlags = drawableProcessor_->GetGeometryRenderFlags(drawableIndex);

    const bool isInside = (drawable->GetDrawableFlags() & drawableFlags_)
        && (drawable->GetViewMask() & viewMask_)
        && (inside || sphere_.IsInsideFast(drawable->GetWorldBoundingBox()));
    const bool isLit = isInside
        && (geometryFlags & GeometryRenderFlag::Lit)
        && (drawable->GetLightMaskInZone() & lightMask_);
    const bool isForwardLit = isLit && (geometryFlags & GeometryRenderFlag::ForwardLit);
    const bool isShadowCaster = shadowCasters_ && isInside
        && drawable->GetCastShadows()
        && (drawable->GetViewMask() & shadowViewMask_)
        && (drawable->GetShadowMask() & lightMask_);
    return { isLit, isForwardLit, isShadowCaster };
}

SpotLightGeometryQuery::SpotLightGeometryQuery(
    ea::vector<Drawable*>& result, bool& hasLitGeometries, ea::vector<Drawable*>* shadowCasters,
    const DrawableProcessor* drawableProcessor, Light* light, unsigned primaryViewMask, unsigned shadowViewMask)
    : FrustumOctreeQuery(result, light->GetFrustum(), DRAWABLE_GEOMETRY, primaryViewMask | shadowViewMask)
    , hasLitGeometries_(hasLitGeometries)
    , shadowCasters_(shadowCasters)
    , drawableProcessor_(drawableProcessor)
    , lightMask_(light->GetLightMaskEffective())
    , shadowViewMask_(shadowViewMask)
{
    hasLitGeometries_ = false;
    if (shadowCasters_)
        shadowCasters_->clear();
}

void SpotLightGeometryQuery::TestDrawables(Drawable** start, Drawable** end, bool inside)
{
    for (Drawable* drawable : MakeIteratorRange(start, end))
    {
        const auto result = IsLitOrShadowCaster(drawable, inside);
        if (result.isLit_)
            hasLitGeometries_ = true;
        if (result.isForwardLit_)
            result_.push_back(drawable);
        if (result.isShadowCaster_)
            shadowCasters_->push_back(drawable);
    }
}

LightGeometryQueryResult SpotLightGeometryQuery::IsLitOrShadowCaster(Drawable* drawable, bool inside) const
{
    const unsigned drawableIndex = drawable->GetDrawableIndex();
    const unsigned geometryFlags = drawableProcessor_->GetGeometryRenderFlags(drawableIndex);

    const bool isInside = (drawable->GetDrawableFlags() & drawableFlags_)
        && (drawable->GetViewMask() & viewMask_)
        && (inside || frustum_.IsInsideFast(drawable->GetWorldBoundingBox()));
    const bool isLit = isInside
        && (geometryFlags & GeometryRenderFlag::Lit)
        && (drawable->GetLightMaskInZone() & lightMask_);
    const bool isForwardLit = isLit && (geometryFlags & GeometryRenderFlag::ForwardLit);
    const bool isShadowCaster = shadowCasters_ && isInside
        && drawable->GetCastShadows()
        && (drawable->GetViewMask() & shadowViewMask_)
        && (drawable->GetShadowMask() & lightMask_);
    return { isLit, isForwardLit, isShadowCaster };
}

DirectionalLightShadowCasterQuery::DirectionalLightShadowCasterQuery(ea::vector<Drawable*>& result,
    const Frustum& frustum, DrawableFlags drawableFlags, Light* light, unsigned viewMask)
    : FrustumOctreeQuery(result, frustum, drawableFlags, viewMask)
    , lightMask_(light->GetLightMask())
{
}

void DirectionalLightShadowCasterQuery::TestDrawables(Drawable** start, Drawable** end, bool inside)
{
    for (Drawable* drawable : MakeIteratorRange(start, end))
    {
        if (IsShadowCaster(drawable, inside))
            result_.push_back(drawable);
    }
}

bool DirectionalLightShadowCasterQuery::IsShadowCaster(Drawable* drawable, bool inside) const
{
    return drawable->GetCastShadows()
        && (drawable->GetDrawableFlags() & drawableFlags_)
        && (drawable->GetViewMask() & viewMask_)
        && (drawable->GetShadowMask() & lightMask_)
        && (inside || frustum_.IsInsideFast(drawable->GetWorldBoundingBox()));
}

}
