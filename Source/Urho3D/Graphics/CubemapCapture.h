//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Precompiled.h"

#include "../Scene/Component.h"
#include "../Graphics/TextureCube.h"

namespace Urho3D
{

class RenderPath;
class TextureCube;

/// %CubemapCapture component facilitates rendering a scene into mip-0 of a cubemap texture. If the internal texture is used then it will be RGBA32F,
/// provide an alternative texture to use a different format. Raises the `CubemapCaptureUpdate` event to facilitate further scheduled tasks such
/// as filter kernels or spherical-harmonic calculation. When set to link to a Zone it may use the Zone found on the same node as the CubemapCapture component
/// or in the immediate parent of it's node. Accepting a Zone from the parent allows the CubemapCapture to be positioned within the Zone at a location
/// other than the Zone's centroid.
class URHO3D_API CubemapCapture : public Component
{
    URHO3D_OBJECT(CubemapCapture, Component);
public:
    /// Construct.
    explicit CubemapCapture(Context*);
    /// Destruct.
    ~CubemapCapture();

    /// Register factory and attributes.
    static void RegisterObject(Context*);

    /// Sets the dirty bit.
    void MarkDirty() { dirty_ = true; }
    /// Sets the size of a cubemap face edge.
    void SetFaceSize(unsigned dim);
    /// Sets the cubemap to render-to.
    void SetTarget(SharedPtr<TextureCube> texTarget);
    /// Sets the render-path to use for face capture.
    void SetRenderPath(SharedPtr<RenderPath> rp);
    /// Sets the maximum render clip distance.
    void SetFarDist(float dist) { farDist_ = dist; MarkDirty(); }
    /// Sets whether to link our texture to a Zone found on the same node.
    void SetZoneLinked(bool state) { matchToZone_ = state; SetupZone(); }

    /// Returns the state of the dirty flag.
    inline bool IsDirty() const { return dirty_; }
    /// Returns the far distance for the rendering.
    inline float GetFarDist() const { return farDist_;  }
    /// Returns the face edge size.
    inline unsigned GetFaceSize() const { return faceSize_; }
    /// Returns the active target cubemap.
    SharedPtr<TextureCube> GetTarget() const;
    /// Returns the filtered cubemap.
    SharedPtr<TextureCube> GetFiltered() const;
    /// Returns the active render-path.
    SharedPtr<RenderPath> GetRenderPath() const;
    /// Returns true if the zone should be using the same texture.
    bool IsZoneLinked() const { return matchToZone_; }

    /// Performs the render from the location of this object's Node. Must be called from outside of the core render-loop.
    void Render();
    /// Runs the Filter_128 function on the target if it exists.
    void Filter();

    /// Utility to render all dirty CubemapCapture components in the given scene. Optionally limit how many to process.
    static void RenderAll(SharedPtr<Scene> scene, unsigned maxCt = UINT_MAX);
    /// Utility for performing the render with the provided objects, renders the component unrequired for bulk work. Use the shouldBeginEndFrame parameter when performing bulk work.
    static void Render(SharedPtr<Scene> scene, SharedPtr<RenderPath> renderPath, SharedPtr<TextureCube> cubeTarget, Vector3 position, float farDist, bool shouldBeginEndFrame = true);
    /// Utility for determining the rotation required to face a cubemap axis for render-capture.
    static Quaternion CubeFaceRotation(CubeMapFace face);

    /// Utility function for blurring cubemaps for use with IBL.
    static SharedPtr<TextureCube> FilterCubemap(SharedPtr<TextureCube> cubeMap, unsigned rayCt);
    /// Utility function for blurring cubemaps for use with IBL.
    static void FilterCubemaps(const eastl::vector< SharedPtr<TextureCube> >& cubemaps, const eastl::vector< SharedPtr<TextureCube> >& destCubemaps, unsigned rayCt) { FilterCubemaps(cubemaps, destCubemaps, eastl::vector<unsigned> { rayCt }); }
    /// Utility function for blurring cubemaps, accepts a list raycast counts to provide the shaders.
    static void FilterCubemaps(const eastl::vector< SharedPtr<TextureCube> >& cubemaps, const eastl::vector< SharedPtr<TextureCube> >& destCubemaps, const eastl::vector<unsigned>& rayCts);
    /// Sensible default setup for bluring 128x128 face cubemaps for IBL, using more rays as roughness increases.
    static void FilterCubemaps_128(const eastl::vector< SharedPtr<TextureCube> >& cubemaps, const eastl::vector< SharedPtr<TextureCube> >& destCubes) { FilterCubemaps(cubemaps, destCubes, { 1, 8, 16, 16, 16, 16, 32, 32 }); }

private:
    /// Set zone to use our target cube.
    void SetupZone();
    /// Constructs the cubemaps if needed.
    void SetupTextures();

    /// Renderpath to use.
    SharedPtr<RenderPath> renderPath_;
    /// Active cubemap target.
    SharedPtr<TextureCube> target_;
    /// Filtered cubemap target.
    SharedPtr<TextureCube> filtered_;
    /// Clip distance limit.
    float farDist_ = { 10000.0f };
    /// Length of cubemap face edge.
    unsigned faceSize_ = { 128 };
    /// Dirty indicator bit for rerender needs.
    bool dirty_ = { true };
    /// Indicates the local cubemap should be the same as that for Zone component in the same node or the parent node.
    bool matchToZone_ = { false };
};

}
