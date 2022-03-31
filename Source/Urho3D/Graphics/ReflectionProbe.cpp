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

extern const char* SCENE_CATEGORY;

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
            if (1 || reflectionProbe->IsDynamic()) // TODO(reflection): Implement me
                dynamicProbes_.push_back(reflectionProbe);
            else
                staticProbes_.push_back(reflectionProbe);
        }
        revision_ = ea::max(1u, revision_ + 1);
    }
}

void ReflectionProbeManager::QueryDynamicProbes(const BoundingBox& worldBoundingBox,
    ea::span<ReflectionProbeReference, 2> probes)
{
    for (ReflectionProbe* probe : dynamicProbes_)
    {
        const BoundingBox localBoundingBox = worldBoundingBox.Transformed(probe->GetWorldToLocal());
        const BoundingBox clippedBoundingBox = localBoundingBox.Clipped(probe->GetBoundingBox());
        if (clippedBoundingBox.Defined())
        {
            const float volume = clippedBoundingBox.Volume() / localBoundingBox.Volume();
            if (!probes[0].data_)
            {
                probes[0].data_ = &probe->GetProbeData();
                probes[0].priority_ = probe->GetPriority();
                probes[0].volume_ = volume;
            }
            else if (probes[0].priority_ < probe->GetPriority() || probes[0].volume_ < volume)
            {
                probes[1] = probes[0];
                probes[0].data_ = &probe->GetProbeData();
                probes[0].priority_ = probe->GetPriority();
                probes[0].volume_ = volume;
            }
        }
    }
}

void ReflectionProbeManager::QueryStaticProbes(const BoundingBox& worldBoundingBox,
    ea::span<ReflectionProbeReference, 2> probes, float& cacheDistanceSquared)
{

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
    worldToLocal_ = node_ ? node_->GetWorldTransform().Inverse() : Matrix3x4::IDENTITY;
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
