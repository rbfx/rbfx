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
#include "../Scene/Node.h"
#include "../Scene/PrefabReader.h"
#include "../Scene/PrefabResource.h"
#include "../Scene/SceneResolver.h"
#include "../Replica/StaticNetworkObject.h"
#include "../Resource/ResourceCache.h"

namespace Urho3D
{

StaticNetworkObject::StaticNetworkObject(Context* context) : NetworkObject(context) {}

StaticNetworkObject::~StaticNetworkObject() = default;

void StaticNetworkObject::RegisterObject(Context* context)
{
    context->AddFactoryReflection<StaticNetworkObject>(Category_Network);

    URHO3D_ACCESSOR_ATTRIBUTE("Client Prefab", GetClientPrefabAttr, SetClientPrefabAttr, ResourceRef, ResourceRef(PrefabResource::GetTypeStatic()), AM_DEFAULT);
}

void StaticNetworkObject::SetClientPrefab(PrefabResource* prefab)
{
    if (prefab && prefab->GetName().empty())
    {
        URHO3D_ASSERTLOG(0, "StaticNetworkObject::SetClientPrefab is called with unnamed resource for object {}",
            ToString(GetNetworkId()));
        return;
    }

    if (!IsStandalone())
    {
        URHO3D_LOGERROR("StaticNetworkObject::SetClientPrefab is called for object {} which is already replicated",
            ToString(GetNetworkId()));
        return;
    }

    clientPrefab_ = prefab;
}

void StaticNetworkObject::InitializeOnServer()
{
    latestSentParentObject_ = GetParentNetworkId();
}

void StaticNetworkObject::WriteSnapshot(NetworkFrame frame, Serializer& dest)
{
    dest.WriteUInt(static_cast<unsigned>(GetParentNetworkId()));
    dest.WriteString(clientPrefab_ ? clientPrefab_->GetName() : EMPTY_STRING);
    dest.WriteString(node_->GetName());

    dest.WriteVector3(node_->GetWorldPosition());
    dest.WritePackedQuaternion(node_->GetWorldRotation());
    dest.WriteVector3(node_->GetSignedWorldScale());
}

bool StaticNetworkObject::PrepareReliableDelta(NetworkFrame frame)
{
    const auto parentObject = GetParentNetworkId();
    const bool needUpdate = latestSentParentObject_ != parentObject;
    latestSentParentObject_ = parentObject;
    return needUpdate;
}

void StaticNetworkObject::WriteReliableDelta(NetworkFrame frame, Serializer& dest)
{
    dest.WriteUInt(static_cast<unsigned>(latestSentParentObject_));
}

void StaticNetworkObject::InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned)
{
    const auto parentNetworkId = static_cast<NetworkId>(src.ReadUInt());
    SetParentNetworkObject(parentNetworkId);

    const ea::string clientPrefabName = src.ReadString();
    SetClientPrefabAttr(ResourceRef{PrefabResource::GetTypeStatic(), clientPrefabName});

    if (clientPrefab_)
    {
        const auto flags = PrefabLoadFlag::KeepExistingComponents;
        PrefabReaderFromMemory reader{clientPrefab_->GetNodePrefab()};
        node_->Load(reader, flags);
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
}

void StaticNetworkObject::ReadReliableDelta(NetworkFrame frame, Deserializer& src)
{
    const auto parentObject = static_cast<NetworkId>(src.ReadUInt());
    SetParentNetworkObject(parentObject);
}

ResourceRef StaticNetworkObject::GetClientPrefabAttr() const
{
    return GetResourceRef(clientPrefab_, PrefabResource::GetTypeStatic());
}

void StaticNetworkObject::SetClientPrefabAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetClientPrefab(cache->GetResource<PrefabResource>(value.name_));
}

}
