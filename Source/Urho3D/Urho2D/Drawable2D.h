// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Graphics/Drawable.h"
#include "../Graphics/GraphicsDefs.h"

namespace Urho3D
{

class Drawable2D;
class Renderer2D;
class Texture2D;
class VertexBuffer;

/// 2D vertex.
struct Vertex2D
{
    /// Position.
    Vector3 position_;
    /// Color.
    unsigned color_;
    /// UV.
    Vector2 uv_;

    /// Equality comparison operator.
    bool operator==(const Vertex2D& other) const
    {
        if (this == &other)
            return true;
        return position_ == other.position_ && color_ == other.color_ && uv_ == other.uv_;
    }

    /// Inequality comparison operator.
    bool operator!=(const Vertex2D& other) const
    {
        return !(*this == other);
    }
};

/// 2D source batch.
struct SourceBatch2D
{
    /// Construct.
    SourceBatch2D();

    /// Owner.
    WeakPtr<Drawable2D> owner_;
    /// Distance to camera.
    mutable float distance_;
    /// Draw order.
    int drawOrder_;
    /// Material.
    SharedPtr<Material> material_;
    /// Vertices.
    ea::vector<Vertex2D> vertices_;

    /// Equality comparison operator.
    bool operator==(const SourceBatch2D& other) const
    {
        if (this == &other)
            return true;
        return owner_ == other.owner_ && distance_ == other.distance_ && drawOrder_ == other.drawOrder_ &&
            material_ == other.material_ && vertices_ == other.vertices_;
    }

    /// Inequality comparison operator.
    bool operator!=(const SourceBatch2D& other) const
    {
        return !(*this == other);
    }
};

/// Pixel size (equal 0.01f).
extern URHO3D_API const float PIXEL_SIZE;

/// Base class for 2D visible components.
class URHO3D_API Drawable2D : public Drawable
{
    URHO3D_OBJECT(Drawable2D, Drawable);

public:
    /// Construct.
    explicit Drawable2D(Context* context);
    /// Destruct.
    ~Drawable2D() override;
    /// Register object factory. Drawable must be registered first.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

    /// Set layer.
    /// @property
    void SetLayer(int layer);
    /// Set order in layer.
    /// @property
    void SetOrderInLayer(int orderInLayer);

    /// Return layer.
    /// @property
    int GetLayer() const { return layer_; }

    /// Return order in layer.
    /// @property
    int GetOrderInLayer() const { return orderInLayer_; }

    /// Return all source batches (called by Renderer2D).
    const ea::vector<SourceBatch2D>& GetSourceBatches();

protected:
    /// Handle scene being assigned.
    void OnSceneSet(Scene* previousScene, Scene* scene) override;
    /// Handle node transform being dirtied.
    void OnMarkedDirty(Node* node) override;
    /// Handle draw order changed.
    virtual void OnDrawOrderChanged() = 0;
    /// Update source batches.
    virtual void UpdateSourceBatches() = 0;

    /// Return draw order by layer and order in layer.
    int GetDrawOrder() const { return layer_ << 16u | orderInLayer_; }

    /// Layer.
    int layer_;
    /// Order in layer.
    int orderInLayer_;
    /// Source batches.
    ea::vector<SourceBatch2D> sourceBatches_;
    /// Source batches dirty flag.
    bool sourceBatchesDirty_;
    /// Renderer2D.
    WeakPtr<Renderer2D> renderer_;
};

}
