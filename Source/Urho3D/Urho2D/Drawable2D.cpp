// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Urho2D/Drawable2D.h"
#include "Urho3D/Urho2D/Renderer2D.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

const float PIXEL_SIZE = 0.01f;

SourceBatch2D::SourceBatch2D() :
    distance_(0.0f),
    drawOrder_(0)
{
}

Drawable2D::Drawable2D(Context* context) :
    Drawable(context, DRAWABLE_GEOMETRY2D),
    layer_(0),
    orderInLayer_(0),
    sourceBatchesDirty_(true)
{
}

Drawable2D::~Drawable2D()
{
    if (renderer_)
        renderer_->RemoveDrawable(this);
}

void Drawable2D::RegisterObject(Context* context)
{
    URHO3D_ACCESSOR_ATTRIBUTE("Layer", GetLayer, SetLayer, int, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Order in Layer", GetOrderInLayer, SetOrderInLayer, int, 0, AM_DEFAULT);
    URHO3D_ATTRIBUTE("View Mask", int, viewMask_, DEFAULT_VIEWMASK, AM_DEFAULT);
}

void Drawable2D::OnSetEnabled()
{
    bool enabled = IsEnabledEffective();

    if (enabled && renderer_)
        renderer_->AddDrawable(this);
    else if (!enabled && renderer_)
        renderer_->RemoveDrawable(this);
}

void Drawable2D::SetLayer(int layer)
{
    if (layer == layer_)
        return;

    layer_ = layer;

    OnDrawOrderChanged();
}

void Drawable2D::SetOrderInLayer(int orderInLayer)
{
    if (orderInLayer == orderInLayer_)
        return;

    orderInLayer_ = orderInLayer;

    OnDrawOrderChanged();
}

const ea::vector<SourceBatch2D>& Drawable2D::GetSourceBatches()
{
    if (sourceBatchesDirty_)
        UpdateSourceBatches();

    return sourceBatches_;
}

void Drawable2D::OnSceneSet(Scene* previousScene, Scene* scene)
{
    // Do not call Drawable::OnSceneSet, as 2D drawable components should not be added to the octree
    // but are instead rendered through Renderer2D
    if (scene)
    {
        renderer_ = scene->GetOrCreateComponent<Renderer2D>();

        if (IsEnabledEffective())
            renderer_->AddDrawable(this);
    }
    else
    {
        if (renderer_)
            renderer_->RemoveDrawable(this);
    }
}

void Drawable2D::OnMarkedDirty(Node* node)
{
    Drawable::OnMarkedDirty(node);

    sourceBatchesDirty_ = true;
}

}
