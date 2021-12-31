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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Network/DefaultNetworkObject.h"
#include "../Scene/Node.h"
#include "../Scene/SceneResolver.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"

namespace Urho3D
{

DefaultNetworkObject::DefaultNetworkObject(Context* context) : NetworkObject(context) {}

DefaultNetworkObject::~DefaultNetworkObject() = default;

void DefaultNetworkObject::RegisterObject(Context* context)
{
    context->RegisterFactory<DefaultNetworkObject>();

    URHO3D_ACCESSOR_ATTRIBUTE("Client Prefab", GetClientPrefabAttr, SetClientPrefabAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
}

void DefaultNetworkObject::SetClientPrefab(XMLFile* prefab)
{
    if (prefab && prefab->GetName().empty())
    {
        URHO3D_ASSERTLOG(0, "DefaultNetworkObject::SetClientPrefab is called with unnamed resource");
        return;
    }

    clientPrefab_ = prefab;
}

void DefaultNetworkObject::InitializeOnServer()
{
    const auto networkManager = GetServerNetworkManager();
    const unsigned traceCapacity = networkManager->GetTraceCapacity();

    lastParentNetworkId_ = GetParentNetworkId();
    worldPositionTrace_.Resize(traceCapacity);
    worldRotationTrace_.Resize(traceCapacity);
}

void DefaultNetworkObject::OnTransformDirty()
{
    worldTransformCounter_ = WorldTransformCooldown;
}

void DefaultNetworkObject::WriteSnapshot(unsigned frame, VectorBuffer& dest)
{
    const auto parentNetworkId = GetParentNetworkId();
    dest.WriteUInt(static_cast<unsigned>(parentNetworkId));

    dest.WriteString(clientPrefab_ ? clientPrefab_->GetName() : EMPTY_STRING);

    dest.WriteString(node_->GetName());

    dest.WriteVector3(node_->GetWorldPosition());
    dest.WritePackedQuaternion(node_->GetWorldRotation());
    dest.WriteVector3(node_->GetSignedWorldScale());
}

bool DefaultNetworkObject::WriteReliableDelta(unsigned frame, VectorBuffer& dest)
{
    const unsigned updateMask = EvaluateReliableDeltaMask();
    if (!updateMask)
        return false;

    dest.WriteUInt(updateMask);
    if (updateMask & UpdateParentNetworkObjectId)
        dest.WriteUInt(static_cast<unsigned>(lastParentNetworkId_));

    return true;
}

bool DefaultNetworkObject::WriteUnreliableDelta(unsigned frame, VectorBuffer& dest)
{
    const Vector3 worldPosition = node_->GetWorldPosition();
    const Quaternion worldRotation = node_->GetWorldRotation();
    worldPositionTrace_.Set(frame, worldPosition);
    worldRotationTrace_.Set(frame, worldRotation);

    const unsigned updateMask = EvaluateUnreliableDeltaMask();
    if (!updateMask)
        return false;

    dest.WriteUInt(updateMask);
    if (updateMask & UpdateWorldTransform)
    {
        dest.WriteVector3(worldPosition);
        dest.WriteQuaternion(worldRotation);
    }

    return true;
}

unsigned DefaultNetworkObject::EvaluateReliableDeltaMask()
{
    unsigned mask = 0;

    const auto parentNetworkId = GetParentNetworkId();
    if (lastParentNetworkId_ != parentNetworkId)
    {
        lastParentNetworkId_ = parentNetworkId;
        mask |= UpdateParentNetworkObjectId;
    }

    return mask;
}

unsigned DefaultNetworkObject::EvaluateUnreliableDeltaMask()
{
    unsigned mask = 0;

    if (worldTransformCounter_ > 0)
    {
        mask |= UpdateWorldTransform;
        --worldTransformCounter_;
    }

    return mask;
}

void DefaultNetworkObject::InterpolateState(const NetworkTime& time)
{
    const ClientNetworkManager* clientNetworkManager = GetClientNetworkManager();
    const unsigned positionExtrapolationFrames = clientNetworkManager->GetPositionExtrapolationFrames();

    if (auto newWorldPosition = worldPositionTrace_.RepairAndSample(time, {positionExtrapolationFrames}))
        node_->SetWorldPosition(*newWorldPosition);

    if (auto newWorldRotation = worldRotationTrace_.RepairAndSample(time))
        node_->SetWorldRotation(*newWorldRotation);
}

void DefaultNetworkObject::ReadSnapshot(unsigned frame, VectorBuffer& src)
{
    const auto parentNetworkId = static_cast<NetworkId>(src.ReadUInt());
    SetParentNetworkObject(parentNetworkId);

    const ea::string clientPrefabName = src.ReadString();
    SetClientPrefabAttr(ResourceRef{XMLFile::GetTypeStatic(), clientPrefabName});

    if (clientPrefab_)
    {
        const XMLElement& prefabRootElement = clientPrefab_->GetRoot();

        SceneResolver resolver;
        unsigned nodeID = prefabRootElement.GetUInt("id");
        resolver.AddNode(nodeID, node_);

        node_->LoadXML(prefabRootElement, resolver, true, true, LOCAL, false);
    }

    node_->SetName(src.ReadString());

    const Vector3 worldPosition = src.ReadVector3();
    const Quaternion worldRotation = src.ReadPackedQuaternion();
    const Vector3 worldScale = src.ReadVector3();
    const Matrix3x4 worldTransform{ worldPosition, worldRotation, worldScale };
    const Matrix3x4 localTransform = node_->IsTransformHierarchyRoot()
        ? worldTransform
        : node_->GetParent()->GetWorldTransform().Inverse() * worldTransform;
    node_->SetTransform(localTransform);

    const auto networkManager = GetClientNetworkManager();
    const unsigned traceCapacity = networkManager->GetTraceCapacity();
    worldPositionTrace_.Resize(traceCapacity);
    worldRotationTrace_.Resize(traceCapacity);
}

void DefaultNetworkObject::ReadReliableDelta(unsigned frame, VectorBuffer& src)
{
    const unsigned updateMask = src.ReadUInt();
    if (updateMask & UpdateParentNetworkObjectId)
    {
        const auto parentNetworkId = static_cast<NetworkId>(src.ReadUInt());
        SetParentNetworkObject(parentNetworkId);
    }
}

void DefaultNetworkObject::ReadUnreliableDelta(unsigned frame, VectorBuffer& src)
{
    const unsigned updateMask = src.ReadUInt();
    if (updateMask & UpdateWorldTransform)
    {
        worldPositionTrace_.Set(frame, src.ReadVector3());
        worldRotationTrace_.Set(frame, src.ReadQuaternion());
    }
}

ResourceRef DefaultNetworkObject::GetClientPrefabAttr() const
{
    return GetResourceRef(clientPrefab_, XMLFile::GetTypeStatic());
}

void DefaultNetworkObject::SetClientPrefabAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetClientPrefab(cache->GetResource<XMLFile>(value.name_));
}

}
