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
#include "../Graphics/Camera.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/ReflectionProbe.h"
#include "../Graphics/TextureCube.h"
#include "../RenderPipeline/RenderPipeline.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Node.h"

namespace Urho3D
{

namespace
{

Quaternion faceRotations[MAX_CUBEMAP_FACES] = {
    Quaternion(0, 90, 0),
    Quaternion(0, -90, 0),
    Quaternion(-90, 0, 0),
    Quaternion(90, 0, 0),
    Quaternion(0, 0, 0),
    Quaternion(0, 180, 0),
};

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

    // If node with probe, store and stop
    if (const auto& probeData = bvh[0].data_)
    {
        intersections.push_back(&bvh[0]);
        return;
    }

    // Check children
    if (bvh.size() > 1)
    {
        URHO3D_ASSERT(bvh.size() % 2 == 1);
        const unsigned stride = bvh.size() / 2;
        QueryBVH(intersections, bvh.subspan(1, stride), worldBoundingBox);
        QueryBVH(intersections, bvh.subspan(1 + stride, stride), worldBoundingBox);
    }
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
    URHO3D_ATTRIBUTE("Render Budget", unsigned, renderBudget_, DefaultRenderBudget, AM_DEFAULT);
}

void ReflectionProbeManager::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    for (ReflectionProbe* reflectionProbe : GetReflectionProbes())
        reflectionProbe->DrawDebugGeometry(debug, depthTest);
}

void ReflectionProbeManager::OnComponentAdded(TrackedComponentBase* baseComponent)
{
    auto probe = static_cast<ReflectionProbe*>(baseComponent);
    MarkReflectionProbeDirty(probe, false);
    probe->AllocateRenderSurfaces();
}

void ReflectionProbeManager::OnComponentRemoved(TrackedComponentBase* baseComponent)
{
    auto probe = static_cast<ReflectionProbe*>(baseComponent);
    MarkReflectionProbeDirty(probe, false);
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
        UpdateArrays();
        BuildBVH(staticProbesBvh_, staticProbes_);
    }

    for (InternalReflectionProbeData& probeData : dynamicProbes_)
        probeData.Update();

    // TODO(reflection): Remove this hack, support different probe types
    if (probesToUpdate_.empty())
    {
        for (const InternalReflectionProbeData& probeData : dynamicProbes_)
            probesToUpdate_.emplace(probeData.probe_);
        for (ReflectionProbe* probe : staticProbes_)
            probesToUpdate_.emplace(probe);
    }

    if (queuedProbes_.empty())
        UpdateQueuedProbes();

    for (const ReflectionProbeFace& probeFace : queuedProbes_)
    {
        if (!probeFace.probe_)
            continue;

        RenderSurface* renderSurface = probeFace.probe_->GetRenderSurface(probeFace.face_);
        renderSurface->QueueUpdate();
    }
    queuedProbes_.clear();

    // TODO: Fix me
    /*if (!renderPipeline_)
    {
        renderPipeline_ = MakeShared<RenderPipeline>(context_);
    }*/
}

void ReflectionProbeManager::UpdateArrays()
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

    revision_ = ea::max(1u, revision_ + 1);
    arraysDirty_ = false;
}

void ReflectionProbeManager::UpdateQueuedProbes()
{
    for (const auto& probe : probesToUpdate_)
    {
        if (!probe)
            continue;

        for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
            queuedProbes_.push_back(ReflectionProbeFace{probe, static_cast<CubeMapFace>(face)});
    }
    probesToUpdate_.clear();
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

void ReflectionProbe::AllocateRenderSurfaces()
{
    if (renderTexture_)
        return;

    // TODO(reflection): Make configurable
    renderTexture_ = MakeShared<TextureCube>(context_);
    renderTexture_->SetSize(256, Graphics::GetRGBAFormat(), TEXTURE_RENDERTARGET);
    for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
    {
        auto cameraNode = MakeShared<Node>(context_);
        cameraNode->SetWorldRotation(faceRotations[face]);
        auto camera = cameraNode->CreateComponent<Camera>();
        camera->SetFov(90.0f);
        camera->SetNearClip(0.01f);
        camera->SetFarClip(100.0f);
        camera->SetAspectRatio(1.0f);
        renderCameras_[face] = cameraNode;

        auto viewport = MakeShared<Viewport>(context_);
        viewport->SetScene(GetScene());
        viewport->SetCamera(camera);

        RenderSurface* surface = renderTexture_->GetRenderSurface(static_cast<CubeMapFace>(face));
        surface->SetViewport(0, viewport);
    }
    UpdateProbeData();
}

void ReflectionProbe::DeallocateRenderSurfaces()
{
    renderTexture_ = nullptr;
}

RenderSurface* ReflectionProbe::GetRenderSurface(CubeMapFace face) const
{
    return renderTexture_ ? renderTexture_->GetRenderSurface(face) : nullptr;
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
    UpdateProbeData();
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

    // TODO(reflection): Extract from here
    if (renderTexture_)
    {
        for (Node* cameraNode : renderCameras_)
            cameraNode->SetWorldPosition(node_->GetWorldPosition());
    }
}

void ReflectionProbe::UpdateProbeData()
{
    // TODO(reflection): Handle SH here too
    data_.reflectionMap_ = texture_ ? texture_ : renderTexture_;
    data_.roughnessToLODFactor_ = data_.reflectionMap_ ? LogBaseTwo(data_.reflectionMap_->GetWidth()) : 1.0f;
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
