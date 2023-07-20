//
// Copyright (c) 2008-2022 the Urho3D project.
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

/// \file

#pragma once

#include "../Core/Mutex.h"
#include "../Graphics/Drawable.h"
#include "../RenderAPI/PipelineState.h"
#include "../Graphics/Viewport.h"
#include "../Math/Color.h"

#include <EASTL/set.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

class RenderPipelineView;
class Geometry;
class Drawable;
class Light;
class Material;
class Pass;
class Technique;
class Octree;
class Graphics;
class RenderSurface;
class ResourceCache;
class Scene;
class Skeleton;
class Technique;
class Texture;
class Texture2D;
class TextureCube;
class Zone;

static const int SHADOW_MIN_PIXELS = 64;
static const int INSTANCING_BUFFER_DEFAULT_SIZE = 1024;

/// Light vertex shader variations.
enum LightVSVariation
{
    LVS_DIR = 0,
    LVS_SPOT,
    LVS_POINT,
    LVS_SHADOW,
    LVS_SPOTSHADOW,
    LVS_POINTSHADOW,
    LVS_SHADOWNORMALOFFSET,
    LVS_SPOTSHADOWNORMALOFFSET,
    LVS_POINTSHADOWNORMALOFFSET,
    MAX_LIGHT_VS_VARIATIONS
};

/// Per-vertex light vertex shader variations.
enum VertexLightVSVariation
{
    VLVS_NOLIGHTS = 0,
    VLVS_1LIGHT,
    VLVS_2LIGHTS,
    VLVS_3LIGHTS,
    VLVS_4LIGHTS,
    MAX_VERTEXLIGHT_VS_VARIATIONS
};

/// Light pixel shader variations.
enum LightPSVariation
{
    LPS_NONE = 0,
    LPS_SPOT,
    LPS_POINT,
    LPS_POINTMASK,
    LPS_SPEC,
    LPS_SPOTSPEC,
    LPS_POINTSPEC,
    LPS_POINTMASKSPEC,
    LPS_SHADOW,
    LPS_SPOTSHADOW,
    LPS_POINTSHADOW,
    LPS_POINTMASKSHADOW,
    LPS_SHADOWSPEC,
    LPS_SPOTSHADOWSPEC,
    LPS_POINTSHADOWSPEC,
    LPS_POINTMASKSHADOWSPEC,
    MAX_LIGHT_PS_VARIATIONS
};

/// Deferred light volume vertex shader variations.
enum DeferredLightVSVariation
{
    DLVS_NONE = 0,
    DLVS_DIR,
    DLVS_ORTHO,
    DLVS_ORTHODIR,
    MAX_DEFERRED_LIGHT_VS_VARIATIONS
};

/// Deferred light volume pixels shader variations.
enum DeferredLightPSVariation
{
    DLPS_NONE = 0,
    DLPS_SPOT,
    DLPS_POINT,
    DLPS_POINTMASK,
    DLPS_SPEC,
    DLPS_SPOTSPEC,
    DLPS_POINTSPEC,
    DLPS_POINTMASKSPEC,
    DLPS_SHADOW,
    DLPS_SPOTSHADOW,
    DLPS_POINTSHADOW,
    DLPS_POINTMASKSHADOW,
    DLPS_SHADOWSPEC,
    DLPS_SPOTSHADOWSPEC,
    DLPS_POINTSHADOWSPEC,
    DLPS_POINTMASKSHADOWSPEC,
    DLPS_SHADOWNORMALOFFSET,
    DLPS_SPOTSHADOWNORMALOFFSET,
    DLPS_POINTSHADOWNORMALOFFSET,
    DLPS_POINTMASKSHADOWNORMALOFFSET,
    DLPS_SHADOWSPECNORMALOFFSET,
    DLPS_SPOTSHADOWSPECNORMALOFFSET,
    DLPS_POINTSHADOWSPECNORMALOFFSET,
    DLPS_POINTMASKSHADOWSPECNORMALOFFSET,
    DLPS_ORTHO,
    DLPS_ORTHOSPOT,
    DLPS_ORTHOPOINT,
    DLPS_ORTHOPOINTMASK,
    DLPS_ORTHOSPEC,
    DLPS_ORTHOSPOTSPEC,
    DLPS_ORTHOPOINTSPEC,
    DLPS_ORTHOPOINTMASKSPEC,
    DLPS_ORTHOSHADOW,
    DLPS_ORTHOSPOTSHADOW,
    DLPS_ORTHOPOINTSHADOW,
    DLPS_ORTHOPOINTMASKSHADOW,
    DLPS_ORTHOSHADOWSPEC,
    DLPS_ORTHOSPOTSHADOWSPEC,
    DLPS_ORTHOPOINTSHADOWSPEC,
    DLPS_ORTHOPOINTMASKSHADOWSPEC,
    DLPS_ORTHOSHADOWNORMALOFFSET,
    DLPS_ORTHOSPOTSHADOWNORMALOFFSET,
    DLPS_ORTHOPOINTSHADOWNORMALOFFSET,
    DLPS_ORTHOPOINTMASKSHADOWNORMALOFFSET,
    DLPS_ORTHOSHADOWSPECNORMALOFFSET,
    DLPS_ORTHOSPOTSHADOWSPECNORMALOFFSET,
    DLPS_ORTHOPOINTSHADOWSPECNORMALOFFSET,
    DLPS_ORTHOPOINTMASKSHADOWSPECNORMALOFFSET,
    MAX_DEFERRED_LIGHT_PS_VARIATIONS
};

/// Skinning mode.
enum SkinningMode
{
    SKINNING_AUTO,
    SKINNING_HARDWARE,
    SKINNING_SOFTWARE,
};

/// Statistics collected during the last frame.
/// TODO: Move other metrics here.
struct FrameStatistics
{
    unsigned animations_{};
    unsigned changedAnimations_{};
};

/// High-level rendering subsystem. Manages drawing of 3D views.
class URHO3D_API Renderer : public Object
{
    URHO3D_OBJECT(Renderer, Object);

public:
    /// Construct.
    explicit Renderer(Context* context);
    /// Destruct.
    ~Renderer() override;

    /// Set backbuffer render surface. Null corresponds to the application backbuffer.
    void SetBackbufferRenderSurface(RenderSurface* renderSurface);
    /// Set number of backbuffer viewports to render.
    /// @property
    void SetNumViewports(unsigned num);
    /// Set a backbuffer viewport.
    /// @property{set_viewports}
    void SetViewport(unsigned index, Viewport* viewport);
    /// Set default non-textured material technique.
    /// @property
    void SetDefaultTechnique(Technique* technique);
    /// Set default texture max anisotropy level.
    /// @property
    void SetTextureAnisotropy(int level);
    /// Set default texture filtering.
    /// @property
    void SetTextureFilterMode(TextureFilterMode mode);
    /// Set texture quality level. See the QUALITY constants in GraphicsDefs.h.
    /// @property
    void SetTextureQuality(MaterialQuality quality);
    /// Set skinning mode.
    void SetSkinningMode(SkinningMode mode);
    /// Set number of bones used for software skinning.
    void SetNumSoftwareSkinningBones(unsigned numBones);

    /// Return number of backbuffer viewports.
    /// @property
    unsigned GetNumViewports() const { return viewports_.size(); }

    /// Return backbuffer viewport by index.
    /// @property{get_viewports}
    Viewport* GetViewport(unsigned index) const;
    /// Return nth backbuffer viewport associated to a scene. Index 0 returns the first.
    Viewport* GetViewportForScene(Scene* scene, unsigned index) const;
    /// Return default non-textured material technique.
    /// @property
    Technique* GetDefaultTechnique() const;

    /// Return default texture max. anisotropy level.
    /// @property
    int GetTextureAnisotropy() const { return textureAnisotropy_; }

    /// Return default texture filtering mode.
    /// @property
    TextureFilterMode GetTextureFilterMode() const { return textureFilterMode_; }

    /// Return texture quality level.
    /// @property
    MaterialQuality GetTextureQuality() const { return textureQuality_; }

    /// Return skinning mode.
    SkinningMode GetSkinningMode() const { return skinningMode_; }

    /// Return whether hardware skinning is used.
    bool GetUseHardwareSkinning() const { return (skinningMode_ == SKINNING_AUTO && hardwareSkinningSupported_) || skinningMode_ == SKINNING_HARDWARE; }

    /// Return number of bones used for software skinning.
    unsigned GetNumSoftwareSkinningBones() const { return numSoftwareSkinningBones_; }

    /// Return number of views rendered.
    /// @property
    unsigned GetNumViews() const { return renderPipelineViews_.size(); }

    /// Return number of geometries rendered.
    /// @property
    unsigned GetNumGeometries() const;
    /// Return number of lights rendered.
    /// @property
    unsigned GetNumLights() const;
    /// Return number of shadow maps rendered.
    /// @property
    unsigned GetNumShadowMaps() const;
    /// Return number of occluders rendered.
    /// @property
    unsigned GetNumOccluders() const;

    /// Return the default zone.
    /// @property
    Zone* GetDefaultZone() const { return defaultZone_; }

    /// Return the default material.
    /// @property
    Material* GetDefaultMaterial() const { return defaultMaterial_; }

    /// Return the default range attenuation texture.
    /// @property
    Texture2D* GetDefaultLightRamp() const { return defaultLightRamp_; }

    /// Return the default spotlight attenuation texture.
    /// @property
    Texture2D* GetDefaultLightSpot() const { return defaultLightSpot_; }

    /// Return completely black 1x1x1 cubemap.
    TextureCube* GetBlackCubeMap() const { return blackCubeMap_; }

    /// Return the frame update parameters.
    const FrameInfo& GetFrameInfo() const { return frame_; }

    /// Return statistics of current frame.
    const FrameStatistics& GetFrameStats() const { return frameStats_; }
    FrameStatistics& GetMutableFrameStats() { return frameStats_; }

    /// Update for rendering. Called by HandleRenderUpdate().
    void Update(float timeStep);
    /// Render. Called by Engine.
    void Render();
    /// Add debug geometry to the debug renderer.
    void DrawDebugGeometry(bool depthTest);
    /// Queue a render surface's viewports for rendering. Called by the surface, or by View.
    void QueueRenderSurface(RenderSurface* renderTarget);
    /// Queue a viewport for rendering. Null surface means backbuffer.
    void QueueViewport(RenderSurface* renderTarget, Viewport* viewport);

    /// Return volume geometry for a light.
    Geometry* GetLightGeometry(Light* light);
    /// Return quad geometry used in postprocessing.
    Geometry* GetQuadGeometry();

private:
    /// Initialize when screen mode initially set.
    void Initialize();
    /// Release shaders used in materials.
    void ReleaseMaterialShaders();
    /// Reload textures.
    void ReloadTextures();
    /// Create light volume geometries.
    void CreateGeometries();
    /// Update a queued viewport for rendering.
    void UpdateQueuedViewport(unsigned index);
    /// Handle screen mode event.
    void HandleScreenMode(StringHash eventType, VariantMap& eventData);
    /// Handle render update event.
    void HandleRenderUpdate(StringHash eventType, VariantMap& eventData);
    void UpdateMousePositionsForMainViewports();

    /// Graphics subsystem.
    WeakPtr<Graphics> graphics_;
    /// Render surface that acts as backbuffer.
    WeakPtr<RenderSurface> backbufferSurface_;
    bool backbufferSurfaceViewportsDirty_{};
    /// Default non-textured material technique.
    SharedPtr<Technique> defaultTechnique_;
    /// Default zone.
    SharedPtr<Zone> defaultZone_;
    /// Directional light quad geometry.
    SharedPtr<Geometry> dirLightGeometry_;
    /// Spot light volume geometry.
    SharedPtr<Geometry> spotLightGeometry_;
    /// Point light volume geometry.
    SharedPtr<Geometry> pointLightGeometry_;
    /// Default material.
    SharedPtr<Material> defaultMaterial_;
    /// Default range attenuation texture.
    SharedPtr<Texture2D> defaultLightRamp_;
    /// Default spotlight attenuation texture.
    SharedPtr<Texture2D> defaultLightSpot_;
    SharedPtr<TextureCube> blackCubeMap_;
    /// Backbuffer viewports.
    ea::vector<SharedPtr<Viewport> > viewports_;
    /// Render surface viewports queued for update.
    ea::vector<ea::pair<WeakPtr<RenderSurface>, WeakPtr<Viewport> > > queuedViewports_;
    /// Render pipeline views that have been processed this frame.
    ea::vector<WeakPtr<RenderPipelineView> > renderPipelineViews_;
    /// Octrees that have been updated during the frame.
    ea::hash_set<Octree*> updatedOctrees_;
    /// Techniques for which missing shader error has been displayed.
    ea::hash_set<Technique*> shaderErrorDisplayed_;
    /// Frame info for rendering.
    FrameInfo frame_;
    /// Texture anisotropy level.
    int textureAnisotropy_{4};
    /// Texture filtering mode.
    TextureFilterMode textureFilterMode_{FILTER_TRILINEAR};
    /// Texture quality level.
    MaterialQuality textureQuality_{QUALITY_HIGH};
    /// Initialized flag.
    bool initialized_{};
    /// Flag for views needing reset.
    bool resetViews_{};
    /// Whether hardware skinning is supported.
    bool hardwareSkinningSupported_{ true };
    /// Skinning mode.
    SkinningMode skinningMode_{};
    /// Number of bones used for software skinning.
    unsigned numSoftwareSkinningBones_{ 4 };

    FrameStatistics frameStats_;
};

}
