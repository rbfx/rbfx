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
#include "../Graphics/GraphicsEvents.h"
#include "../Graphics/ReflectionProbe.h"
#include "../Graphics/TextureCube.h"
#include "../IO/FileSystem.h"
#include "../RenderAPI/RenderDevice.h"
#include "../RenderPipeline/RenderPipeline.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLElement.h"
#include "../Resource/XMLFile.h"
#include "../Scene/Node.h"

namespace Urho3D
{

namespace
{

static const ea::vector<ea::string> reflectionProbeTypeNames = {
    "Baked",
    "Mixed",
    "Dynamic",
    "Custom Texture",
};

bool SupportFiltering(RenderDevice* device)
{
    return device && device->GetCaps().computeShaders_
        && device->IsUnorderedAccessFormatSupported(TextureFormat::TEX_FORMAT_RGBA8_UNORM);
}

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
        return clippedBoundingBox.Volume() / ea::max(localBoundingBox.Volume(), M_EPSILON);
    return ea::nullopt;
}

ReflectionProbeManager::ReflectionProbeManager(Context* context)
    : TrackedComponentRegistryBase(context, ReflectionProbe::GetTypeStatic())
{
    SubscribeToEvent(E_DEVICERESET, &ReflectionProbeManager::RestoreCubemaps);
}

ReflectionProbeManager::~ReflectionProbeManager()
{

}

void ReflectionProbeManager::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ReflectionProbeManager>();

    URHO3D_ACTION_STATIC_LABEL("Bake!", QueueBakeAll, "Renders all baked reflection probes");

    URHO3D_ATTRIBUTE("Query Padding", float, queryPadding_, DefaultQueryPadding, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Render Budget", unsigned, renderBudget_, DefaultRenderBudget, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Filter Cubemaps", bool, filterCubemaps_, true, AM_DEFAULT);
}

void ReflectionProbeManager::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    for (ReflectionProbe* reflectionProbe : GetReflectionProbes())
        reflectionProbe->DrawDebugGeometry(debug, depthTest, true);
}

void ReflectionProbeManager::OnSceneSet(Scene* scene)
{
    if (scene)
        cubemapRenderer_ = MakeShared<CubemapRenderer>(scene);
    else
        cubemapRenderer_ = nullptr;
}

void ReflectionProbeManager::OnComponentAdded(TrackedComponentBase* baseComponent)
{
    auto probe = static_cast<ReflectionProbe*>(baseComponent);
    MarkProbeDirty(probe);

    if (probe->IsRenderOnWake())
        probe->QueueRender();
}

void ReflectionProbeManager::OnComponentRemoved(TrackedComponentBase* baseComponent)
{
    auto probe = static_cast<ReflectionProbe*>(baseComponent);
    MarkProbeDirty(probe);
}

void ReflectionProbeManager::MarkProbeDirty(ReflectionProbe* reflectionProbe)
{
    spatial_.dirty_ = true;
    autoQueue_.dirty_ = true;
}

void ReflectionProbeManager::MarkProbeTransformDirty(ReflectionProbe* reflectionProbe)
{
    if (!reflectionProbe->IsMovable())
    {
        spatial_.dirty_ = true;
        autoQueue_.dirty_ = true;
    }
}

void ReflectionProbeManager::MarkProbeRealtimeDirty(ReflectionProbe* reflectionProbe)
{
    autoQueue_.dirty_ = true;
}

void ReflectionProbeManager::QueueProbeUpdate(ReflectionProbe* reflectionProbe)
{
    if (reflectionProbe->GetProbeType() != ReflectionProbeType::CustomTexture)
        probesToUpdate_.emplace(reflectionProbe);
}

void ReflectionProbeManager::QueueBakeAll()
{
    for (ReflectionProbe* reflectionProbe : GetReflectionProbes())
        QueueProbeUpdate(reflectionProbe);
}

void ReflectionProbeManager::Update()
{
    if (spatial_.dirty_)
        UpdateSpatialCache();

    if (autoQueue_.dirty_)
        UpdateAutoQueueCache();

    for (InternalReflectionProbeData& probeData : spatial_.movableProbes_)
        probeData.Update();

    for (ReflectionProbe* probe : autoQueue_.realtimeProbes_)
        QueueProbeUpdate(probe);

    if (updateQueue_.empty())
        FillUpdateQueue();

    ConsumeUpdateQueue();
}

void ReflectionProbeManager::UpdateSpatialCache()
{
    spatial_.immovableProbes_.clear();
    spatial_.movableProbes_.clear();

    for (ReflectionProbe* reflectionProbe : GetReflectionProbes())
    {
        if (reflectionProbe->IsMovable())
            spatial_.movableProbes_.emplace_back(reflectionProbe);
        else
            spatial_.immovableProbes_.push_back(reflectionProbe);
    }

    spatial_.revision_ = ea::max(1u, spatial_.revision_ + 1);
    spatial_.dirty_ = false;

    BuildBVH(spatial_.immovableProbesBvh_, spatial_.immovableProbes_);
}

void ReflectionProbeManager::UpdateAutoQueueCache()
{
    autoQueue_.realtimeProbes_.clear();

    for (ReflectionProbe* reflectionProbe : GetReflectionProbes())
    {
        if (reflectionProbe->IsRealtimeUpdate())
            autoQueue_.realtimeProbes_.push_back(reflectionProbe);
    }

    autoQueue_.dirty_ = false;
}

void ReflectionProbeManager::FillUpdateQueue()
{
    for (const auto& probe : probesToUpdate_)
    {
        if (!probe)
            continue;

        WeakPtr<CubemapRenderer> cubemapRenderer{probe->GetCubemapRenderer()};
        updateQueue_.push_back(QueuedReflectionProbe{probe, ea::move(cubemapRenderer)});
    }
    probesToUpdate_.clear();
}

void ReflectionProbeManager::ConsumeUpdateQueue()
{
    auto renderDevice = GetSubsystem<RenderDevice>();
    const bool filterCubemapsEffective = filterCubemaps_ && SupportFiltering(renderDevice);

    unsigned numStaticProbesRendered = 0;
    unsigned numRenderedFaces = 0;
    for (QueuedReflectionProbe& queuedProbe : updateQueue_)
    {
        if (numRenderedFaces >= renderBudget_ && renderBudget_ > 0)
            break;

        ReflectionProbe* probe = queuedProbe.probe_;
        if (!probe)
            continue;

        const Vector3 position = probe->GetNode()->GetWorldPosition();

        // Skip custom texture probes
        if (probe->GetProbeType() == ReflectionProbeType::CustomTexture)
        {
            queuedProbe = {};
            continue;
        }

        // Render dynamic probe
        if (CubemapRenderer* probeRenderer = queuedProbe.cubemapRenderer_)
        {
            CubemapUpdateParameters params;
            params.settings_ = probe->GetCubemapRenderingSettings();
            params.position_ = position;
            params.slicedUpdate_ = probe->IsSlicedUpdate();
            params.filterResult_ = filterCubemapsEffective;

            const CubemapUpdateResult result = probeRenderer->Update(params);

            numRenderedFaces += result.numRenderedFaces_;
            if (result.isComplete_)
                queuedProbe = {};
            continue;
        }

        // Skip static probes if out of budget
        if (numStaticProbesRendered >= MaxStaticUpdates)
            continue;

        // Render mixed probe
        if (TextureCube* probeTexture = probe->GetMixedProbeTexture())
        {
            // TODO: Create correct texture from the start
            const TextureFlags probeFlags = probeTexture->GetParams().flags_;
            if (filterCubemapsEffective && !probeFlags.Test(TextureFlag::BindUnorderedAccess))
            {
                probeTexture->SetSize(probeTexture->GetWidth(), probeTexture->GetFormat(),
                    probeFlags | TextureFlag::BindUnorderedAccess);
            }

            CubemapUpdateParameters params;
            params.settings_ = probe->GetCubemapRenderingSettings();
            params.position_ = position;
            params.filterResult_ = filterCubemapsEffective;
            params.overrideFinalTexture_ = probeTexture;

            const CubemapUpdateResult result = cubemapRenderer_->Update(params);
            URHO3D_ASSERT(result.isComplete_);
        }
        else
        {
            // Render baked probe
            CubemapUpdateParameters params;
            params.settings_ = probe->GetCubemapRenderingSettings();
            params.position_ = position;
            params.filterResult_ = filterCubemapsEffective;

            const CubemapUpdateResult result = cubemapRenderer_->Update(params);
            URHO3D_ASSERT(result.isComplete_);

            cubemapRenderer_->OnCubemapRendered.Subscribe(this,
                [probe](ReflectionProbeManager* self, TextureCube* texture)
            {
                const ea::string filePath = self->GetBakedProbeFilePath();
                const ea::string fileName = Format("ReflectionProbe-{}", probe->GetID());
                const ea::string textureFileName = self->SaveTextureToFile(texture, filePath, fileName);

                probe->SetTextureAttr(ResourceRef{TextureCube::GetTypeStatic(), textureFileName});
                return false;
            });
        }

        numRenderedFaces += MAX_CUBEMAP_FACES;
        queuedProbe = {};
        ++numStaticProbesRendered;
    }

    ea::erase_if(updateQueue_, [](const QueuedReflectionProbe& queuedProbe)
    {
        return !queuedProbe.probe_;
    });
}

void ReflectionProbeManager::RestoreCubemaps()
{
    for (ReflectionProbe* reflectionProbe : GetReflectionProbes())
    {
        if (reflectionProbe->IsRenderOnWake())
            QueueProbeUpdate(reflectionProbe);
    }
}

ea::string ReflectionProbeManager::GetBakedProbeFilePath() const
{
    if (Scene* scene = GetScene())
    {
        const ea::string sceneFileName = scene->GetFileName();
        if (!sceneFileName.empty())
            return Format("{}.d/Textures", sceneFileName);
    }
    return ea::string();
}

ea::string ReflectionProbeManager::SaveTextureToFile(
    TextureCube* texture, const ea::string& filePath, const ea::string& fileName)
{
    if (filePath.empty())
    {
        URHO3D_LOGERROR("Cannot save reflection probe texture to file");
        return EMPTY_STRING;
    }

    const ea::string textureFileName = Format("{}/{}.xml", filePath, fileName);

    // TODO(compute): Save mips?
    auto xmlFile = MakeShared<XMLFile>(context_);
    XMLElement rootElement = xmlFile->GetOrCreateRoot("cubemap");
    for (unsigned face = 0; face < MAX_CUBEMAP_FACES; ++face)
    {
        const ea::string faceFileName = Format("{}-{}.png", fileName, face);

        auto image = texture->GetImage(static_cast<CubeMapFace>(face));
        image->SavePNG(Format("{}/{}", filePath, faceFileName));

        XMLElement faceElement = rootElement.CreateChild("face");
        faceElement.SetAttribute("name", faceFileName);
    }
    xmlFile->SaveFile(textureFileName);

    return textureFileName;
}

void ReflectionProbeManager::QueryDynamicProbes(const BoundingBox& worldBoundingBox,
    ea::span<ReflectionProbeReference, 2> probes) const
{
    for (const InternalReflectionProbeData& probeData : spatial_.movableProbes_)
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
    thread_local ea::vector<const ReflectionProbeBVH*> tempIntersectedProbes;
    auto& intersectedProbes = tempIntersectedProbes;
    intersectedProbes.clear();

    for (ReflectionProbeReference& ref : probes)
        ref.Reset();

    QueryBVH(intersectedProbes, spatial_.immovableProbesBvh_, worldBoundingBox.Padded(Vector3::ONE * queryPadding_));

    float cacheDistance = queryPadding_;
    for (const ReflectionProbeBVH* node : intersectedProbes)
    {
        const float signedDistance = node->worldBoundingBox_.SignedDistanceToBoundingBox(worldBoundingBox);
        if (signedDistance > 0.0f)
            cacheDistance = ea::min(cacheDistance, signedDistance);
        else
        {
            const InternalReflectionProbeData& probeData = *node->data_;
            if (const auto volume = probeData.GetIntersectionVolume(worldBoundingBox))
            {
                const ReflectionProbeReference newReference{probeData.data_, probeData.priority_, *volume};
                AppendReference(probes, newReference);
            }

            cacheDistance = ea::min(cacheDistance, -signedDistance);
        }
    }
    cacheDistanceSquared = cacheDistance * cacheDistance;
}

ReflectionProbe::ReflectionProbe(Context* context)
    : TrackedComponent<TrackedComponentBase, ReflectionProbeManager>(context)
{
}

ReflectionProbe::~ReflectionProbe()
{
}

void ReflectionProbe::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ReflectionProbe>(Category_Scene);

    URHO3D_ACTION_STATIC_LABEL("Render!", QueueRender, "Renders cubemap for reflection probe");

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Movable", IsMovable, SetMovable, bool, false, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Probe Type", GetProbeType, SetProbeType, ReflectionProbeType, reflectionProbeTypeNames, ReflectionProbeType::Baked, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Realtime Update", IsRealtimeUpdate, SetRealtimeUpdate, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Sliced Update", IsSlicedUpdate, SetSlicedUpdate, bool, false, AM_DEFAULT);

    URHO3D_ATTRIBUTE_EX("Bounding Box Min", Vector3, boundingBox_.min_, MarkTransformDirty, -Vector3::ONE, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Bounding Box Max", Vector3, boundingBox_.max_, MarkTransformDirty, Vector3::ONE, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Priority", GetPriority, SetPriority, int, 0, AM_DEFAULT);

    URHO3D_ACCESSOR_ATTRIBUTE("Texture", GetTextureAttr, SetTextureAttr, ResourceRef, ResourceRef(TextureCube::GetTypeStatic()), AM_DEFAULT);

    URHO3D_ACCESSOR_ATTRIBUTE("Use Box Projection", IsBoxProjectionUsed, SetBoxProjectionUsed, bool, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Projection Box Min", Vector3, projectionBox_.min_, UpdateProbeBoxData, -Vector3::ONE, AM_DEFAULT);
    URHO3D_ATTRIBUTE_EX("Projection Box Max", Vector3, projectionBox_.max_, UpdateProbeBoxData, Vector3::ONE, AM_DEFAULT);

    URHO3D_ACCESSOR_ATTRIBUTE("Texture Size", GetTextureSize, SetTextureSize, unsigned, CubemapRenderingSettings::DefaultTextureSize, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("View Mask", GetViewMask, SetViewMask, unsigned, CubemapRenderingSettings::DefaultViewMask, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Near Clip", GetNearClip, SetNearClip, float, CubemapRenderingSettings::DefaultNearClip, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Far Clip", GetFarClip, SetFarClip, float, CubemapRenderingSettings::DefaultFarClip, AM_DEFAULT);
}

void ReflectionProbe::DrawDebugGeometry(DebugRenderer* debug, bool depthTest, bool compact)
{
    if (debug && IsEnabledEffective())
    {
        debug->AddBoundingBox(boundingBox_, node_->GetWorldTransform(), Color::BLUE, depthTest);
        if (!compact && useBoxProjection_)
            debug->AddBoundingBox(data_.projectionBox_, Matrix3x4::IDENTITY, Color::MAGENTA, depthTest);
    }
}

void ReflectionProbe::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    DrawDebugGeometry(debug, depthTest, false);
}

void ReflectionProbe::QueueRender()
{
    if (ReflectionProbeManager* manager = GetRegistry())
        manager->QueueProbeUpdate(this);
}

void ReflectionProbe::SetMovable(bool movable)
{
    if (movable_ != movable)
    {
        movable_ = movable;
        MarkComponentDirty();
    }
}

void ReflectionProbe::SetProbeType(ReflectionProbeType probeType)
{
    if (probeType_ != probeType)
    {
        probeType_ = probeType;
        UpdateCubemapRenderer();

        if (IsRenderOnWake())
            QueueRender();
    }
}

void ReflectionProbe::SetRealtimeUpdate(bool realtimeUpdate)
{
    if (realtimeUpdate_ != realtimeUpdate)
    {
        realtimeUpdate_ = realtimeUpdate;
        MarkRealtimeDirty();
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
    if (probeType_ == ReflectionProbeType::Baked || probeType_ == ReflectionProbeType::CustomTexture)
        UpdateProbeTextureData(texture_);
}

void ReflectionProbe::SetTextureAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetTexture(cache->GetResource<TextureCube>(value.name_));
}

void ReflectionProbe::SetBoxProjectionUsed(bool useBoxProjection)
{
    if (useBoxProjection_ != useBoxProjection)
    {
        useBoxProjection_ = useBoxProjection;
        UpdateProbeBoxData();
    }
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

void ReflectionProbe::SetTextureSize(unsigned value)
{
    if (cubemapRenderingSettings_.textureSize_ != value)
    {
        cubemapRenderingSettings_.textureSize_ = value;
        if (mixedProbeTexture_)
        {
            CubemapRenderer::DefineTexture(mixedProbeTexture_, cubemapRenderingSettings_);
            UpdateProbeTextureData(mixedProbeTexture_);
        }
    }
}

void ReflectionProbe::SetViewMask(unsigned value)
{
    if (cubemapRenderingSettings_.viewMask_ != value)
    {
        cubemapRenderingSettings_.viewMask_ = value;
    }
}

void ReflectionProbe::SetNearClip(float value)
{
    if (cubemapRenderingSettings_.nearClip_ != value)
    {
        cubemapRenderingSettings_.nearClip_ = value;
    }
}

void ReflectionProbe::SetFarClip(float value)
{
    if (cubemapRenderingSettings_.farClip_ != value)
    {
        cubemapRenderingSettings_.farClip_ = value;
    }
}

void ReflectionProbe::MarkComponentDirty()
{
    if (ReflectionProbeManager* manager = GetRegistry())
        manager->MarkProbeDirty(this);
}

void ReflectionProbe::MarkTransformDirty()
{
    if (ReflectionProbeManager* manager = GetRegistry())
        manager->MarkProbeTransformDirty(this);

    UpdateProbeBoxData();
}

void ReflectionProbe::MarkRealtimeDirty()
{
    if (ReflectionProbeManager* manager = GetRegistry())
        manager->MarkProbeRealtimeDirty(this);
}

void ReflectionProbe::OnDynamicCubemapRendered(TextureCube* texture)
{
    UpdateProbeTextureData(texture);
}

void ReflectionProbe::UpdateCubemapRenderer()
{
    Scene* scene = GetScene();
    if (!scene || probeType_ == ReflectionProbeType::Baked || probeType_ == ReflectionProbeType::CustomTexture)
    {
        dynamicProbeRenderer_ = nullptr;
        mixedProbeTexture_ = nullptr;
        UpdateProbeTextureData(texture_);
        return;
    }

    switch (probeType_)
    {
    case ReflectionProbeType::Dynamic:
        if (!dynamicProbeRenderer_)
        {
            dynamicProbeRenderer_ = MakeShared<CubemapRenderer>(scene);
            dynamicProbeRenderer_->OnCubemapRendered.Subscribe(this, &ReflectionProbe::OnDynamicCubemapRendered);
            mixedProbeTexture_ = nullptr;
        }
        break;
    case ReflectionProbeType::Mixed:
        if (!mixedProbeTexture_)
        {
            mixedProbeTexture_ = MakeShared<TextureCube>(context_);
            CubemapRenderer::DefineTexture(mixedProbeTexture_, cubemapRenderingSettings_);
            dynamicProbeRenderer_ = nullptr;
            UpdateProbeTextureData(mixedProbeTexture_);
        }
        break;
    default:
        assert(!"Unsupported probe type");
        break;
    }
}

void ReflectionProbe::UpdateProbeTextureData(TextureCube* texture)
{
    data_.reflectionMap_ = texture;
    data_.roughnessToLODFactor_ = texture ? LogBaseTwo(texture->GetWidth()) : 1.0f;
}

void ReflectionProbe::UpdateProbeBoxData()
{
    if (!useBoxProjection_ || !node_)
    {
        data_.cubemapCenter_ = Vector4::ZERO;
        return;
    }

    const Vector3 position = node_->GetWorldPosition();
    data_.cubemapCenter_ = position.ToVector4(1.0);
    data_.projectionBox_.min_ = position + projectionBox_.min_;
    data_.projectionBox_.max_ = position + projectionBox_.max_;
}

void ReflectionProbe::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (node_)
    {
        node_->AddListener(this);
        MarkTransformDirty();
    }

    UpdateCubemapRenderer();
}

void ReflectionProbe::OnMarkedDirty(Node* node)
{
    MarkTransformDirty();
}

}
