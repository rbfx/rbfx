//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "../Container/Ptr.h"
#include "../Graphics/Drawable.h"
#include "../Graphics/Material.h"
#include "../Math/MathDefs.h"
#include "../Math/Matrix3x4.h"
#include "../Math/Rect.h"

#if URHO3D_SPHERICAL_HARMONICS
#include "../Math/SphericalHarmonics.h"
#endif

namespace Urho3D
{

class Camera;
class Drawable;
class Geometry;
class Light;
class Material;
class Matrix3x4;
class Pass;
class ShaderVariation;
class Texture2D;
class VertexBuffer;
class View;
class Zone;
struct LightBatchQueue;

/// Per-instance shader parameters.
struct InstanceShaderParameters
{
#if URHO3D_SPHERICAL_HARMONICS
    /// L2 spherical harmonics for ambient light.
    SphericalHarmonicsDot9 ambient_;
#else
    /// Constant ambient light.
    Vector4 ambient_;
#endif
};

/// Container of shaders used in a batch.
struct BatchShaders
{
    /// Vertex shader.
    ShaderVariation* vertexShader_;
    /// Pixel shader.
    ShaderVariation* pixelShader_;
#if !defined(GL_ES_VERSION_2_0) && !defined(URHO3D_D3D9)
    /// Geometry shader.
    ShaderVariation* geometryShader_;
    /// Hull/TCS shader.
    ShaderVariation* hullShader_;
    /// Domain/TES shader.
    ShaderVariation* domainShader_;
#endif
};

/// Queued 3D geometry draw call.
struct Batch
{
    /// Construct with defaults.
    Batch() = default;

    /// Construct from a drawable's source batch.
    explicit Batch(const SourceBatch& rhs) :
        distance_(rhs.distance_),
        renderOrder_(rhs.material_ ? rhs.material_->GetRenderOrder() : DEFAULT_RENDER_ORDER),
        isBase_(false),
        geometry_(rhs.geometry_),
        material_(rhs.material_.Get()),
        worldTransform_(rhs.worldTransform_),
        numWorldTransforms_(rhs.numWorldTransforms_),
        instancingData_(rhs.instancingData_),
        lightQueue_(nullptr),
        geometryType_(rhs.geometryType_),
        lightmapScaleOffset_(rhs.lightmapScaleOffset_),
        lightmapIndex_(rhs.lightmapIndex_)
    {
    }

    /// Calculate state sorting key, which consists of base pass flag, light, pass and geometry.
    void CalculateSortKey();
    /// Prepare for rendering.
    void Prepare(View* view, Camera* camera, bool setModelTransform, bool allowDepthWrite) const;
    /// Prepare and draw.
    void Draw(View* view, Camera* camera, bool allowDepthWrite) const;

    /// State sorting key.
    unsigned long long sortKey_{};
    /// Distance from camera.
    float distance_{};
    /// 8-bit render order modifier from material.
    unsigned char renderOrder_{};
    /// 8-bit light mask for stencil marking in deferred rendering.
    unsigned char lightMask_{};
    /// Base batch flag. This tells to draw the object fully without light optimizations.
    bool isBase_{};
    /// Geometry.
    Geometry* geometry_{};
    /// Material.
    Material* material_{};
    /// World transform(s). For a skinned model, these are the bone transforms.
    const Matrix3x4* worldTransform_{};
    /// Number of world transforms.
    unsigned numWorldTransforms_{};
    /// Per-instance data. If not null, must contain enough data to fill instancing buffer.
    void* instancingData_{};
    /// Zone.
    Zone* zone_{};
    /// Light properties.
    LightBatchQueue* lightQueue_{};
    /// Material pass.
    Pass* pass_{};
    /// Set of shaders used for the batch.
    BatchShaders shaders_;
    /// %Geometry type.
    GeometryType geometryType_{};
    /// Mandatory per-instance shader parameters.
    InstanceShaderParameters shaderParameters_;
    /// Lightmap scale and offset.
    Vector4* lightmapScaleOffset_{};
    /// Lightmap index.
    unsigned lightmapIndex_{};
};

/// Data for one geometry instance.
struct InstanceData
{
    /// Construct undefined.
    InstanceData() = default;

    /// Construct with transform, instancing data and distance.
    InstanceData(const Matrix3x4* worldTransform, const InstanceShaderParameters& shaderParameters,
        const void* instancingData, float distance) :
        worldTransform_(worldTransform),
        shaderParameters_(shaderParameters),
        instancingData_(instancingData),
        distance_(distance)
    {
    }

    /// World transform.
    const Matrix3x4* worldTransform_{};
    /// Mandatory per-instance shader parameters.
    InstanceShaderParameters shaderParameters_;
    /// Instancing data buffer.
    const void* instancingData_{};
    /// Distance from camera.
    float distance_{};
};

/// Instanced 3D geometry draw call.
struct BatchGroup : public Batch
{
    /// Construct with defaults.
    BatchGroup() :
        startIndex_(M_MAX_UNSIGNED)
    {
    }

    /// Construct from a batch.
    explicit BatchGroup(const Batch& batch) :
        Batch(batch),
        startIndex_(M_MAX_UNSIGNED)
    {
    }

    /// Destruct.
    ~BatchGroup() = default;

    /// Add world transform(s) from a batch.
    void AddTransforms(const Batch& batch)
    {
        InstanceData newInstance;
        newInstance.distance_ = batch.distance_;
        newInstance.instancingData_ = batch.instancingData_;
        newInstance.shaderParameters_ = batch.shaderParameters_;

        for (unsigned i = 0; i < batch.numWorldTransforms_; ++i)
        {
            newInstance.worldTransform_ = &batch.worldTransform_[i];
            instances_.push_back(newInstance);
        }
    }

    /// Pre-set the instance data. Buffer must be big enough to hold all data.
    void SetInstancingData(void* lockedData, unsigned stride, unsigned& freeIndex);
    /// Prepare and draw.
    void Draw(View* view, Camera* camera, bool allowDepthWrite) const;

    /// Instance data.
    ea::vector<InstanceData> instances_;
    /// Instance stream start index, or M_MAX_UNSIGNED if transforms not pre-set.
    unsigned startIndex_;
};

/// Instanced draw call grouping key.
struct BatchGroupKey
{
    /// Construct undefined.
    BatchGroupKey() = default;

    /// Construct from a batch.
    explicit BatchGroupKey(const Batch& batch) :
        zone_(batch.zone_),
        lightQueue_(batch.lightQueue_),
        pass_(batch.pass_),
        material_(batch.material_),
        geometry_(batch.geometry_),
        renderOrder_(batch.renderOrder_)
    {
    }

    /// Zone.
    Zone* zone_;
    /// Light properties.
    LightBatchQueue* lightQueue_;
    /// Material pass.
    Pass* pass_;
    /// Material.
    Material* material_;
    /// Geometry.
    Geometry* geometry_;
    /// 8-bit render order modifier from material.
    unsigned char renderOrder_;

    /// Test for equality with another batch group key.
    bool operator ==(const BatchGroupKey& rhs) const
    {
        return zone_ == rhs.zone_ && lightQueue_ == rhs.lightQueue_ && pass_ == rhs.pass_ && material_ == rhs.material_ &&
               geometry_ == rhs.geometry_ && renderOrder_ == rhs.renderOrder_;
    }

    /// Test for inequality with another batch group key.
    bool operator !=(const BatchGroupKey& rhs) const
    {
        return zone_ != rhs.zone_ || lightQueue_ != rhs.lightQueue_ || pass_ != rhs.pass_ || material_ != rhs.material_ ||
               geometry_ != rhs.geometry_ || renderOrder_ != rhs.renderOrder_;
    }

    /// Return hash value.
    unsigned ToHash() const;
};

/// Queue that contains both instanced and non-instanced draw calls.
struct BatchQueue
{
public:
    /// Clear for new frame by clearing all groups and batches.
    void Clear(int maxSortedInstances);
    /// Sort non-instanced draw calls back to front.
    void SortBackToFront();
    /// Sort instanced and non-instanced draw calls front to back.
    void SortFrontToBack();
    /// Sort batches front to back while also maintaining state sorting.
    template <class T> void SortFrontToBack2Pass(ea::vector<T>& batches);
    /// Pre-set instance data of all groups. The vertex buffer must be big enough to hold all data.
    void SetInstancingData(void* lockedData, unsigned stride, unsigned& freeIndex);
    /// Draw.
    void Draw(View* view, Camera* camera, bool markToStencil, bool usingLightOptimization, bool allowDepthWrite) const;
    /// Return the combined amount of instances.
    unsigned GetNumInstances() const;

    /// Return whether the batch group is empty.
    bool IsEmpty() const { return batches_.empty() && batchGroups_.empty(); }

    /// Instanced draw calls.
    ea::unordered_map<BatchGroupKey, BatchGroup> batchGroups_;
    /// Shader remapping table for 2-pass state and distance sort.
    ea::unordered_map<unsigned, unsigned> shaderRemapping_;
    /// Material remapping table for 2-pass state and distance sort.
    ea::unordered_map<unsigned short, unsigned short> materialRemapping_;
    /// Geometry remapping table for 2-pass state and distance sort.
    ea::unordered_map<unsigned short, unsigned short> geometryRemapping_;

    /// Unsorted non-instanced draw calls.
    ea::vector<Batch> batches_;
    /// Sorted non-instanced draw calls.
    ea::vector<Batch*> sortedBatches_;
    /// Sorted instanced draw calls.
    ea::vector<BatchGroup*> sortedBatchGroups_;
    /// Maximum sorted instances.
    unsigned maxSortedInstances_;
    /// Whether the pass command contains extra shader defines.
    bool hasExtraDefines_;

    /// Shader definitions container.
    struct ExtraShaderDefines
    {
        /// Text of the preprocessor definitions.
        ea::string defines_;
        /// Hash of the preprocessor definitions.
        StringHash hash_;
    };

    /// Vertex shader extra defines.
    ExtraShaderDefines vsExtraDefines_;
    /// Pixel shader extra defines.
    ExtraShaderDefines psExtraDefines_;
#if !defined(GL_ES_VERSION_2_0) && !defined(URHO3D_D3D9)
    /// Geometry shader extra defines.
    ExtraShaderDefines gsExtraDefines_;
    /// Hull/TCS shader extra defines.
    ExtraShaderDefines hsExtraDefines_;
    /// Domain/TES shader extra defines.
    ExtraShaderDefines dsExtraDefines_;
#endif
};

/// Queue for shadow map draw calls.
struct ShadowBatchQueue
{
    /// Shadow map camera.
    Camera* shadowCamera_{};
    /// Shadow map viewport.
    IntRect shadowViewport_;
    /// Shadow caster draw calls.
    BatchQueue shadowBatches_;
    /// Directional light cascade near split distance.
    float nearSplit_{};
    /// Directional light cascade far split distance.
    float farSplit_{};
};

/// Queue for light related draw calls.
struct LightBatchQueue
{
    /// Per-pixel light.
    Light* light_;
    /// Light negative flag.
    bool negative_;
    /// Shadow map depth texture.
    Texture2D* shadowMap_;
    /// Lit geometry draw calls, base (replace blend mode).
    BatchQueue litBaseBatches_;
    /// Lit geometry draw calls, non-base (additive).
    BatchQueue litBatches_;
    /// Shadow map split queues.
    ea::vector<ShadowBatchQueue> shadowSplits_;
    /// Per-vertex lights.
    ea::vector<Light*> vertexLights_;
    /// Light volume draw calls.
    ea::vector<Batch> volumeBatches_;
};

}
