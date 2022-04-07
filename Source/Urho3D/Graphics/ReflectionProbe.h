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
#include "../Graphics/ReflectionProbeData.h"
#include "../Math/BoundingBox.h"
#include "../Scene/Component.h"
#include "../Scene/TrackedComponent.h"

#include <EASTL/optional.h>

namespace Urho3D
{

class TextureCube;
class ReflectionProbe;

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
    using ReflectionProbeSpan = TransformedSpan<TrackedComponentBase* const, ReflectionProbe* const, StaticCaster<ReflectionProbe* const>>;
    static constexpr float DefaultQueryPadding = 2.0f;

    explicit ReflectionProbeManager(Context* context);
    ~ReflectionProbeManager() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Mark reflection probe as dirty, i.e. position or dimensions changed.
    void MarkReflectionProbeDirty(ReflectionProbe* reflectionProbe, bool transformOnly);
    /// Update reflection probes if dirty.
    void Update();

    /// Query two most important reflection probes.
    void QueryDynamicProbes(const BoundingBox& worldBoundingBox, ea::span<ReflectionProbeReference, 2> probes) const;
    void QueryStaticProbes(const BoundingBox& worldBoundingBox, ea::span<ReflectionProbeReference, 2> probes,
        float& cacheDistanceSquared) const;

    /// Return all reflection probes.
    ReflectionProbeSpan GetReflectionProbes() const { return StaticCastSpan<ReflectionProbe* const>(GetTrackedComponents()); }
    unsigned GetRevision() const { return revision_; }
    bool HasStaticProbes() const { return !staticProbes_.empty(); }
    bool HasDynamicProbes() const { return !dynamicProbes_.empty(); }

protected:
    void OnComponentAdded(TrackedComponentBase* baseComponent) override;
    void OnComponentRemoved(TrackedComponentBase* baseComponent) override;

private:
    bool arraysDirty_{};
    ea::vector<InternalReflectionProbeData> dynamicProbes_;
    ea::vector<ReflectionProbe*> staticProbes_;
    ea::vector<ReflectionProbeBVH> staticProbesBvh_;
    unsigned revision_{};

    float queryPadding_{DefaultQueryPadding};
};

/// Reflection probe component that specifies reflection applied within a region.
class URHO3D_API ReflectionProbe : public TrackedComponent<ReflectionProbeManager, EnabledOnlyTag>
{
    URHO3D_OBJECT(ReflectionProbe, TrackedComponentBase);

public:
    explicit ReflectionProbe(Context* context);
    ~ReflectionProbe() override;
    static void RegisterObject(Context* context);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    /// Manage properties.
    /// @{
    void SetDynamic(bool dynamic);
    bool IsDynamic() const { return dynamic_; }
    void SetBoundingBox(const BoundingBox& box);
    const BoundingBox& GetBoundingBox() const { return boundingBox_; }
    void SetTexture(TextureCube* texture);
    TextureCube* GetTexture() const { return texture_; }
    void SetTextureAttr(const ResourceRef& value);
    ResourceRef GetTextureAttr() const;
    void SetPriority(int priority);
    int GetPriority() const { return priority_; }
    void SetApproximationColor(const Color& value);
    Color GetApproximationColor() const;
    /// @}

    const ReflectionProbeData& GetProbeData() const { return data_; }

protected:
    void OnNodeSet(Node* node) override;
    void OnMarkedDirty(Node* node) override;

private:
    void MarkComponentDirty();
    void MarkTransformDirty();

    bool dynamic_{};
    BoundingBox boundingBox_{-Vector3::ONE, Vector3::ONE};
    SharedPtr<TextureCube> texture_;
    int priority_{};

    ReflectionProbeData data_;
};


}
