// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Scene/Component.h"
#include "Urho3D/Urho2D/TileMapDefs2D.h"

namespace Urho3D
{

class TileMapLayer2D;
class TmxFile2D;

/// Tile map component.
class URHO3D_API TileMap2D : public Component
{
    URHO3D_OBJECT(TileMap2D, Component);

public:
    /// Construct.
    explicit TileMap2D(Context* context);
    /// Destruct.
    ~TileMap2D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Visualize the component as debug geometry.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Set tmx file.
    /// @property
    void SetTmxFile(TmxFile2D* tmxFile);
    /// Add debug geometry to the debug renderer.
    void DrawDebugGeometry();

    /// Return tmx file.
    /// @property
    TmxFile2D* GetTmxFile() const;

    /// Return information.
    /// @property
    const TileMapInfo2D& GetInfo() const { return info_; }

    /// Return number of layers.
    /// @property
    unsigned GetNumLayers() const { return layers_.size(); }

    /// Return tile map layer at index.
    TileMapLayer2D* GetLayer(unsigned index) const;
    /// Convert tile index to position.
    Vector2 TileIndexToPosition(int x, int y) const;
    /// Convert position to tile index, if out of map return false.
    bool PositionToTileIndex(int& x, int& y, const Vector2& position) const;

    /// Set tile map file attribute.
    void SetTmxFileAttr(const ResourceRef& value);
    /// Return tile map file attribute.
    ResourceRef GetTmxFileAttr() const;
    ///
    ea::vector<SharedPtr<TileMapObject2D> > GetTileCollisionShapes(unsigned gid) const;
private:
    /// Tmx file.
    SharedPtr<TmxFile2D> tmxFile_;
    /// Tile map information.
    TileMapInfo2D info_{};
    /// Root node for tile map layer.
    SharedPtr<Node> rootNode_;
    /// Tile map layers.
    ea::vector<WeakPtr<TileMapLayer2D> > layers_;
};

}
