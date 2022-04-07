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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/ReflectionProbe.h"
#include "../Graphics/TextureCube.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Node.h"

namespace Urho3D
{

namespace
{

void AppendReference(ea::span<ReflectionProbeReference, 2> result, const ReflectionProbeReference& newReference)
{
    if (!result[0] || newReference.IsMoreImportantThan(result[0]))
    {
        // Insert first
        result[1] = result[0];
        result[0] = newReference;
    }
    else if (!result[1] || newReference.IsMoreImportantThan(result[1]))
    {
        // Insert second
        result[1] = newReference;
    }
    // Drop
}

void SplitBoundingVolumes(ea::span<ReflectionProbeBVH> result, ea::span<const InternalReflectionProbeData*> probes)
{
    URHO3D_ASSERT(!probes.empty());

    const unsigned capacity = result.size();
    URHO3D_ASSERT(capacity > 0);
    URHO3D_ASSERT(IsPowerOfTwo(capacity + 1));

    if (probes.size() == 1)
    {
        result[0].data_.emplace(*probes[0]);
        result[0].worldBoundingBox_ = result[0].data_->GetWorldBoundingBox();
        return;
    }

    result[0].data_.reset();
    result[0].worldBoundingBox_ = BoundingBox{};
    for (const InternalReflectionProbeData* probeData : probes)
        result[0].worldBoundingBox_.Merge(probeData->GetWorldBoundingBox());

    const Vector3 size = result[0].worldBoundingBox_.Size();
    const int splitAxis = size.x_ > size.y_ && size.x_ > size.z_ ? 0 : size.y_ > size.z_ ? 1 : 2;
    ea::sort(probes.begin(), probes.end(),
        [splitAxis](const InternalReflectionProbeData* lhs, const InternalReflectionProbeData* rhs)
    {
        const Vector3 lhsCenter = lhs->GetWorldBoundingBox().Center();
        const Vector3 rhsCenter = rhs->GetWorldBoundingBox().Center();
        return lhsCenter.Data()[splitAxis] < rhsCenter.Data()[splitAxis];
    });

    const unsigned median = probes.size() / 2;
    SplitBoundingVolumes(result.subspan(1u, capacity / 2),
        probes.subspan(0, median));
    SplitBoundingVolumes(result.subspan(1u + capacity / 2, capacity / 2),
        probes.subspan(median, probes.size() - median));
}

void BuildBVH(ea::vector<ReflectionProbeBVH>& result, const ea::vector<ReflectionProbe*>& probes)
{
    result.clear();
    if (probes.empty())
        return;

    const unsigned numProbes = probes.size();

    ea::vector<InternalReflectionProbeData> probesData;
    probesData.reserve(numProbes);
    for (ReflectionProbe* probe : probes)
        probesData.emplace_back(probe);

    ea::vector<const InternalReflectionProbeData*> probesDataPtrs;
    probesDataPtrs.reserve(numProbes);
    for (const InternalReflectionProbeData& probeData : probesData)
        probesDataPtrs.push_back(&probeData);

    result.resize(NextPowerOfTwo(numProbes) * 2 - 1);
    SplitBoundingVolumes(result, probesDataPtrs);
}

void QueryBVH(ea::vector<const ReflectionProbeBVH*>& intersections,
    ea::span<const ReflectionProbeBVH> bvh, const BoundingBox& worldBoundingBox)
{
    // Early return if outside
    if (bvh[0].worldBoundingBox_.IsInside(worldBoundingBox) == OUTSIDE)
        return;

    // Check leafs
    if (bvh.size() == 1)
    {
        if (const auto& probeData = bvh[0].data_)
            intersections.push_back(&bvh[0]);
        return;
    }

    // Check children
    URHO3D_ASSERT(bvh.size() % 2 == 1);
    const unsigned stride = bvh.size() / 2;
    QueryBVH(intersections, bvh.subspan(1, stride), worldBoundingBox);
    QueryBVH(intersections, bvh.subspan(1 + stride, stride), worldBoundingBox);
}

}

extern const char* SCENE_CATEGORY;

InternalReflectionProbeData::InternalReflectionProbeData(ReflectionProbe* probe)
    : probe_(probe)
    , data_(&probe_->GetProbeData())
    , priority_(probe_->GetPriority())
{
    Update();
}

void InternalReflectionProbeData::Update()
{
    Node* node = probe_->GetNode();
    const Matrix3x4& worldTransform = node->GetWorldTransform();
    worldToLocal_ = worldTransform.Inverse();
    localBoundingBox_ = probe_->GetBoundingBox();
    worldBoundingBox_ = probe_->GetBoundingBox().Transformed(worldTransform);
}

ea::optional<float> InternalReflectionProbeData::GetIntersectionVolume(const BoundingBox& worldBoundingBox) const
{
    const BoundingBox localBoundingBox = worldBoundingBox.Transformed(worldToLocal_);
    const BoundingBox clippedBoundingBox = localBoundingBox.Clipped(localBoundingBox_);
    if (clippedBoundingBox.Defined())
        return clippedBoundingBox.Volume() / localBoundingBox.Volume();
    return ea::nullopt;
}

ReflectionProbeManager::ReflectionProbeManager(Context* context)
    : TrackedComponentRegistryBase(context, ReflectionProbe::GetTypeStatic())
{
}

ReflectionProbeManager::~ReflectionProbeManager()
{

}

void ReflectionProbeManager::RegisterObject(Context* context)
{
    context->RegisterFactory<ReflectionProbeManager>();

    URHO3D_ATTRIBUTE("Query Padding", float, queryPadding_, DefaultQueryPadding, AM_DEFAULT);
}

void ReflectionProbeManager::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    for (ReflectionProbe* reflectionProbe : GetReflectionProbes())
        reflectionProbe->DrawDebugGeometry(debug, depthTest);
}

void ReflectionProbeManager::OnComponentAdded(TrackedComponentBase* baseComponent)
{
    MarkReflectionProbeDirty(static_cast<ReflectionProbe*>(baseComponent), false);
}

void ReflectionProbeManager::OnComponentRemoved(TrackedComponentBase* baseComponent)
{
    MarkReflectionProbeDirty(static_cast<ReflectionProbe*>(baseComponent), false);
}

void ReflectionProbeManager::MarkReflectionProbeDirty(ReflectionProbe* reflectionProbe, bool transformOnly)
{
    if (!reflectionProbe->IsDynamic() || !transformOnly)
        arraysDirty_ = true;
}

void ReflectionProbeManager::Update()
{
    if (arraysDirty_)
    {
        staticProbes_.clear();
        dynamicProbes_.clear();
        for (ReflectionProbe* reflectionProbe : GetReflectionProbes())
        {
            if (reflectionProbe->IsDynamic())
                dynamicProbes_.emplace_back(reflectionProbe);
            else
                staticProbes_.push_back(reflectionProbe);
        }

        BuildBVH(staticProbesBvh_, staticProbes_);

        revision_ = ea::max(1u, revision_ + 1);
        arraysDirty_ = false;
    }

    for (InternalReflectionProbeData& probeData : dynamicProbes_)
        probeData.Update();
}

void ReflectionProbeManager::QueryDynamicProbes(const BoundingBox& worldBoundingBox,
    ea::span<ReflectionProbeReference, 2> probes) const
{
    for (const InternalReflectionProbeData& probeData : dynamicProbes_)
    {
        if (const auto volume = probeData.GetIntersectionVolume(worldBoundingBox))
        {
            const ReflectionProbeReference newReference{probeData.data_, probeData.priority_, *volume};
            AppendReference(probes, newReference);
        }
    }
}

void ReflectionProbeManager::QueryStaticProbes(const BoundingBox& worldBoundingBox,
    ea::span<ReflectionProbeReference, 2> probes, float& cacheDistanceSquared) const
{
    static thread_local ea::vector<const ReflectionProbeBVH*> tempIntersectedProbes;
    auto& intersectedProbes = tempIntersectedProbes;
    intersectedProbes.clear();

    for (ReflectionProbeReference& ref : probes)
        ref.Reset();

    QueryBVH(intersectedProbes, staticProbesBvh_, worldBoundingBox.Padded(Vector3::ONE * queryPadding_));

    cacheDistanceSquared = queryPadding_;
    for (const ReflectionProbeBVH* node : intersectedProbes)
    {
        const float signedDistance = node->worldBoundingBox_.SignedDistanceToBoundingBox(worldBoundingBox);
        if (signedDistance > 0.0f)
            cacheDistanceSquared = ea::min(cacheDistanceSquared, signedDistance);
        else
        {
            const InternalReflectionProbeData& probeData = *node->data_;
            if (const auto volume = probeData.GetIntersectionVolume(worldBoundingBox))
            {
                const ReflectionProbeReference newReference{probeData.data_, probeData.priority_, *volume};
                AppendReference(probes, newReference);
            }

            cacheDistanceSquared = ea::min(cacheDistanceSquared, -signedDistance);
        }
    }
}

ReflectionProbe::ReflectionProbe(Context* context)
    : TrackedComponent<ReflectionProbeManager, EnabledOnlyTag>(context)
{
}

ReflectionProbe::~ReflectionProbe()
{
}

void ReflectionProbe::RegisterObject(Context* context)
{
    context->RegisterFactory<ReflectionProbe>(SCENE_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Dynamic", IsDynamic, SetDynamic, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bounding Box Min", Vector3, boundingBox_.min_, MarkTransformDirty, -Vector3::ONE, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bounding Box Max", Vector3, boundingBox_.max_, MarkTransformDirty, Vector3::ONE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Texture", GetTextureAttr, SetTextureAttr, ResourceRef, ResourceRef(TextureCube::GetTypeStatic()), AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Priority", GetPriority, SetPriority, int, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Approximation Color", GetApproximationColor, SetApproximationColor, Color, Color::BLACK, AM_DEFAULT);
}

void ReflectionProbe::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
        debug->AddBoundingBox(boundingBox_, node_->GetWorldTransform(), Color::BLUE, depthTest);
}

void ReflectionProbe::SetDynamic(bool dynamic)
{
    if (dynamic_ != dynamic)
    {
        dynamic_ = dynamic;
        MarkComponentDirty();
    }
}

void ReflectionProbe::SetBoundingBox(const BoundingBox& box)
{
    if (boundingBox_ != box)
    {
        boundingBox_ = box;
        MarkTransformDirty();
    }
}

void ReflectionProbe::SetTexture(TextureCube* texture)
{
    texture_ = texture;
    data_.reflectionMap_ = texture_;
    data_.roughnessToLODFactor_ = data_.reflectionMap_ ? LogBaseTwo(data_.reflectionMap_->GetWidth()) : 1.0f;
}

void ReflectionProbe::SetTextureAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetTexture(cache->GetResource<TextureCube>(value.name_));
}

ResourceRef ReflectionProbe::GetTextureAttr() const
{
    return GetResourceRef(texture_, TextureCube::GetTypeStatic());
}

void ReflectionProbe::SetPriority(int priority)
{
    if (priority_ != priority)
    {
        priority_ = priority;
        MarkComponentDirty();
    }
}

void ReflectionProbe::SetApproximationColor(const Color& value)
{
    data_.reflectionMapSH_ = SphericalHarmonicsDot9(value);
}

Color ReflectionProbe::GetApproximationColor() const
{
    return Color(data_.reflectionMapSH_.EvaluateAverage());
}

void ReflectionProbe::MarkComponentDirty()
{
    if (ReflectionProbeManager* manager = GetRegistry())
        manager->MarkReflectionProbeDirty(this, false);
}

void ReflectionProbe::MarkTransformDirty()
{
    if (ReflectionProbeManager* manager = GetRegistry())
        manager->MarkReflectionProbeDirty(this, true);
}

void ReflectionProbe::OnNodeSet(Node* node)
{
    if (node)
    {
        node->AddListener(this);
        MarkTransformDirty();
    }
}

void ReflectionProbe::OnMarkedDirty(Node* node)
{
    MarkTransformDirty();
}

}
