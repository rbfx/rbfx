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
#include "../IO/Log.h"
#if defined(URHO3D_PHYSICS) || defined(URHO3D_URHO2D)
#include "../Physics/PhysicsEvents.h"
#endif
#include "../Network/NetworkObject.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"

namespace Urho3D
{

NetworkObject::NetworkObject(Context* context) : Component(context) {}

NetworkObject::~NetworkObject() = default;

void NetworkObject::RegisterObject(Context* context)
{
    context->RegisterFactory<NetworkObject>();
}

void NetworkObject::OnSceneSet(Scene* scene)
{
    if (networkManager_)
    {
        networkManager_->RemoveComponent(this);
        networkManager_ = nullptr;
    }

    if (scene)
    {
        networkManager_ = scene->GetNetworkManager();
        networkManager_->AddComponent(this);
    }
}

bool NetworkObject::IsRelevantForClient(AbstractConnection* connection)
{
    return true;
}

void NetworkObject::OnRemovedOnClient()
{
    if (node_)
        node_->Remove();
}

void NetworkObject::WriteSnapshot(VectorBuffer& dest)
{
}

bool NetworkObject::WriteReliableDelta(VectorBuffer& dest)
{
    return false;
}

void NetworkObject::ReadSnapshot(VectorBuffer& src)
{
}

void NetworkObject::ReadReliableDelta(VectorBuffer& src)
{
}

}
