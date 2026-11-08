// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#pragma once

#include "AssetViewerHelpers.h"

namespace Urho3D
{

/// Lighting mode of the viewer.
enum class LightingMode
{
    /// Use the viewer rig when the scene has no light of its own.
    Auto,
    /// Use only the lights of the scene.
    SceneLights,
    /// Always use the viewer rig.
    ViewerRig,
};

/// Temporary lights, zone, skybox, ground and debug helpers of the viewer.
/// The class owns only weak references to the nodes. The scene owns the nodes.
class ViewerEnvironment
{
public:
    /// Attach the environment to a new scene. Drop the old nodes.
    void Attach(Scene* scene, Camera* camera);

    void SetLightingMode(LightingMode mode);
    LightingMode GetLightingMode() const { return lightingMode_; }
    void SetSkybox(bool enabled);
    bool GetSkybox() const { return skybox_; }
    void SetGrid(bool enabled) { grid_ = enabled; }
    bool GetGrid() const { return grid_; }
    void SetGround(bool enabled);
    bool GetGround() const { return ground_; }
    void SetBounds(bool enabled) { bounds_ = enabled; }
    bool GetBounds() const { return bounds_; }
    void SetWireframe(bool enabled);
    bool GetWireframe() const { return wireframe_; }

    /// Return the name of a lighting mode for the menu.
    static const char* GetLightingModeName(LightingMode mode);
    /// Return true when the scene has a light that does not belong to the viewer rig.
    bool SceneHasOwnLights() const;
    /// Return true when the viewer rig lights are on.
    bool IsRigActive() const;
    /// Return true when the node or one of its parents is a temporary node of the environment.
    bool IsEnvironmentNode(const Node* node) const;
    /// Update all nodes after the content or a setting changes.
    void Update(const BoundingBox& contentBounds);
    /// Turn the rig toward the camera. Call this every frame before the render.
    void UpdateRigDirection();
    /// Make the viewer zone cover the box.
    void ExpandZone(const BoundingBox& box);
    /// Draw the grid and the bounds.
    void DrawDebug(DebugRenderer* debug);

private:
    void UpdateLights();
    void UpdateZone();
    void UpdateSkybox();
    void UpdateGround();
    Node* CreateTemporaryNode(const char* name);

    WeakPtr<Scene> scene_;
    WeakPtr<Camera> camera_;
    WeakPtr<Node> rigNode_;
    WeakPtr<Node> keyLightNode_;
    WeakPtr<Node> fillLightNode_;
    WeakPtr<Node> zoneNode_;
    WeakPtr<Node> skyboxNode_;
    WeakPtr<Node> groundNode_;
    BoundingBox contentBounds_;

    LightingMode lightingMode_{LightingMode::Auto};
    bool skybox_{false};
    bool grid_{true};
    bool ground_{false};
    bool bounds_{false};
    bool wireframe_{false};
};

} // namespace Urho3D
