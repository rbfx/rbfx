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

/// \file

#pragma once

#include "../Container/TransformedSpan.h"
#include "../Graphics/CubemapRenderer.h"
#include "../Graphics/ReflectionProbeData.h"
#include "../Math/BoundingBox.h"
#include "../Scene/Component.h"
#include "../Scene/TrackedComponent.h"

#include <EASTL/optional.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

class TextureCube;
class ReflectionProbe;
class RenderPipeline;

/// Cached internal structure for reflection probe search.
class InternalReflectionProbeData
{
public:
    explicit InternalReflectionProbeData(ReflectionProbe* probe);

    void Update();

    ea::optional<float> GetIntersectionVolume(const BoundingBox& worldBoundingBox) const;
    const BoundingBox& GetWorldBoundingBox() const { return worldBoundingBox_; }

public:
    ReflectionProbe* const probe_{};
    const ReflectionProbeData* const data_{};
    const int priority_{};

private:
    Matrix3x4 worldToLocal_{};
    BoundingBox localBoundingBox_{};
    BoundingBox worldBoundingBox_{};
};

/// Node of static reflection probes tree.
struct ReflectionProbeBVH
{
    BoundingBox worldBoundingBox_;
    ea::optional<InternalReflectionProbeData> data_;
};

/// Reflection probe manager.
class URHO3D_API ReflectionProbeManager : public TrackedComponentRegistryBase
{
    URHO3D_OBJECT(ReflectionProbeManager, TrackedComponentRegistryBase);

public:
    static constexpr bool IsOnlyEnabledTracked = true;

    using ReflectionProbeSpan = TransformedSpan<TrackedComponentBase* const, ReflectionProbe* const, StaticCaster<ReflectionProbe* const>>;
    static constexpr float DefaultQueryPadding = 2.0f;
    static constexpr unsigned DefaultRenderBudget = 6;

    explicit ReflectionProbeManager(Context* context);
    ~ReflectionProbeManager() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Mark reflection probe as dirty, i.e. position or dimensions changed.
    void MarkProbeDirty(ReflectionProbe* reflectionProbe);
    void MarkProbeTransformDirty(ReflectionProbe* reflectionProbe);
    void MarkProbeRealtimeDirty(ReflectionProbe* reflectionProbe);
    /// Queue reflection probe rendering as soon as possible.
    void QueueProbeUpdate(ReflectionProbe* reflectionProbe);
    /// Queue rendering of all Baked reflection probes.
    void QueueBakeAll();

    /// Update reflection probes if dirty. Usually called internally once per frame.
    void Update();

    /// Query two most important reflection probes.
    void QueryDynamicProbes(const BoundingBox& worldBoundingBox, ea::span<ReflectionProbeReference, 2> probes) const;
    void QueryStaticProbes(const BoundingBox& worldBoundingBox, ea::span<ReflectionProbeReference, 2> probes,
        float& cacheDistanceSquared) const;

    /// Return all reflection probes.
    ReflectionProbeSpan GetReflectionProbes() const { return StaticCastSpan<ReflectionProbe* const>(GetTrackedComponents()); }
    unsigned GetRevision() const { return spatial_.revision_; }
    bool HasStaticProbes() const { return !spatial_.immovableProbes_.empty(); }
    bool HasDynamicProbes() const { return !spatial_.movableProbes_.empty(); }

protected:
    void OnSceneSet(Scene* scene) override;
    void OnComponentAdded(TrackedComponentBase* baseComponent) override;
    void OnComponentRemoved(TrackedComponentBase* baseComponent) override;

private:
    void UpdateSpatialCache();
    void UpdateAutoQueueCache();
    void FillUpdateQueue();
    void ConsumeUpdateQueue();

    void RestoreCubemaps();

    ea::string GetBakedProbeFilePath() const;
    ea::string SaveTextureToFile(TextureCube* texture, const ea::string& filePath, const ea::string& fileName);

    struct QueuedReflectionProbe
    {
        WeakPtr<ReflectionProbe> probe_;
        WeakPtr<CubemapRenderer> cubemapRenderer_;
    };

    struct SpatialCache
    {
        bool dirty_{};
        unsigned revision_{};

        ea::vector<InternalReflectionProbeData> movableProbes_;
        ea::vector<ReflectionProbe*> immovableProbes_;
        ea::vector<ReflectionProbeBVH> immovableProbesBvh_;
    } spatial_;

    struct AutoQueueCache
    {
        bool dirty_{};
        ea::vector<ReflectionProbe*> realtimeProbes_;
    } autoQueue_;

    static constexpr unsigned MaxStaticUpdates = 1;

    SharedPtr<CubemapRenderer> cubemapRenderer_;

    float queryPadding_{DefaultQueryPadding};
    unsigned renderBudget_{DefaultRenderBudget};
    bool filterCubemaps_{true};

    ea::unordered_set<WeakPtr<ReflectionProbe>> probesToUpdate_;
    ea::vector<QueuedReflectionProbe> updateQueue_;
    ea::vector<QueuedReflectionProbe> frameUpdates_;
};

/// Type of reflection probe.
enum class ReflectionProbeType
{
    Baked,
    Mixed,
    Dynamic,
    CustomTexture,
};

/// Reflection probe component that specifies reflection applied within a region.
class URHO3D_API ReflectionProbe : public TrackedComponent<TrackedComponentBase, ReflectionProbeManager>
{
    URHO3D_OBJECT(ReflectionProbe, TrackedComponentBase);

public:
    explicit ReflectionProbe(Context* context);
    ~ReflectionProbe() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest, bool compact);
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Queue ReflectionProbe to be re-rendered as soon as possible.
    void QueueRender();

    /// Return cubemap renderer, available for dynamic probes.
    CubemapRenderer* GetCubemapRenderer() const { return dynamicProbeRenderer_; }
    /// Return writeable cubemap, available for mixed probes.
    TextureCube* GetMixedProbeTexture() const { return mixedProbeTexture_; }

    /// Manage properties.
    /// @{
    void SetMovable(bool movable);
    bool IsMovable() const { return movable_; }
    void SetProbeType(ReflectionProbeType probeType);
    ReflectionProbeType GetProbeType() const { return probeType_; }
    void SetRealtimeUpdate(bool realtimeUpdate);
    bool IsRealtimeUpdate() const { return realtimeUpdate_; }
    void SetSlicedUpdate(bool slicedUpdate) { slicedUpdate_ = slicedUpdate; }
    bool IsSlicedUpdate() const { return slicedUpdate_; }

    void SetBoundingBox(const BoundingBox& box);
    const BoundingBox& GetBoundingBox() const { return boundingBox_; }
    void SetPriority(int priority);
    int GetPriority() const { return priority_; }

    void SetTexture(TextureCube* texture);
    TextureCube* GetTexture() const { return texture_; }
    void SetTextureAttr(const ResourceRef& value);
    ResourceRef GetTextureAttr() const;

    void SetBoxProjectionUsed(bool useBoxProjection);
    bool IsBoxProjectionUsed() const { return useBoxProjection_; }
    void SetProjectionBox(const BoundingBox& box);
    const BoundingBox& GetProjectionBox() const { return projectionBox_; }

    const CubemapRenderingSettings& GetCubemapRenderingSettings() const { return cubemapRenderingSettings_; }
    void SetTextureSize(unsigned value);
    unsigned GetTextureSize() const { return cubemapRenderingSettings_.textureSize_; }
    void SetViewMask(unsigned value);
    unsigned GetViewMask() const { return cubemapRenderingSettings_.viewMask_; }
    void SetNearClip(float value);
    float GetNearClip() const { return cubemapRenderingSettings_.nearClip_; }
    void SetFarClip(float value);
    float GetFarClip() const { return cubemapRenderingSettings_.farClip_; }
    /// @}

    const ReflectionProbeData& GetProbeData() const { return data_; }
    bool IsRenderOnWake() const { return probeType_ == ReflectionProbeType::Mixed || probeType_ == ReflectionProbeType::Dynamic; }

protected:
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
    void OnMarkedDirty(Node* node) override;

private:
    void MarkComponentDirty();
    void MarkTransformDirty();
    void MarkRealtimeDirty();

    void OnDynamicCubemapRendered(TextureCube* texture);

    void UpdateCubemapRenderer();
    void UpdateProbeTextureData(TextureCube* texture);
    void UpdateProbeBoxData();

    bool movable_{};
    ReflectionProbeType probeType_{};
    bool realtimeUpdate_{};
    bool slicedUpdate_{};

    BoundingBox boundingBox_{-Vector3::ONE, Vector3::ONE};
    int priority_{};

    bool useBoxProjection_{};
    BoundingBox projectionBox_{-Vector3::ONE, Vector3::ONE};

    SharedPtr<TextureCube> texture_;

    CubemapRenderingSettings cubemapRenderingSettings_;

    ReflectionProbeData data_;

    SharedPtr<CubemapRenderer> dynamicProbeRenderer_;
    SharedPtr<TextureCube> mixedProbeTexture_;
};


}
