//
// Copyright (c) 2008-2022 the Urho3D project.
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
#include "../Core/StringUtils.h"
#include "../Resource/JSONValue.h"
#include "../Scene/Component.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#ifdef URHO3D_PHYSICS
#include "../Physics/PhysicsWorld.h"
#endif
#ifdef URHO3D_PHYSICS2D
#include "../Physics2D/PhysicsWorld2D.h"
#endif

#include "../DebugNew.h"

#ifdef _MSC_VER
#pragma warning(disable:6293)
#endif

namespace Urho3D
{

const char* autoRemoveModeNames[] = {
    "Disabled",
    "Component",
    "Node",
    nullptr
};

Component::Component(Context* context) :
    Serializable(context),
    node_(nullptr),
    id_(0),
    networkUpdate_(false),
    enabled_(true)
{
}

Component::~Component() = default;

AttributeScopeHint Component::GetEffectiveScopeHint() const
{
    // Don't use Serializable::GetReflection() to avoid effect from overrides.
    // GetEffectiveScopeHint() should not depend on the state of the Component
    const ObjectReflection* reflection = context_->GetReflection(GetType());
    return reflection ? reflection->GetEffectiveScopeHint() : AttributeScopeHint::Attribute;
}

bool Component::Save(Serializer& dest) const
{
    // Write type and ID
    if (!dest.WriteStringHash(GetType()))
        return false;
    if (!dest.WriteUInt(id_))
        return false;

    // Write attributes
    return Serializable::Save(dest);
}

bool Component::SaveXML(XMLElement& dest) const
{
    // Write type and ID
    if (!dest.SetString("type", GetTypeName()))
        return false;
    if (!dest.SetUInt("id", id_))
        return false;

    // Write attributes
    return Serializable::SaveXML(dest);
}

bool Component::SaveJSON(JSONValue& dest) const
{
    // Write type and ID
    dest.Set("type", GetTypeName());
    dest.Set("id", id_);

    // Write attributes
    return Serializable::SaveJSON(dest);
}

void Component::GetDependencyNodes(ea::vector<Node*>& dest)
{
}

void Component::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
}

void Component::SetEnabled(bool enable)
{
    if (enable != enabled_)
    {
        enabled_ = enable;
        OnSetEnabled();

        // Send change event for the component
        Scene* scene = GetScene();
        if (scene)
        {
            using namespace ComponentEnabledChanged;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_SCENE] = scene;
            eventData[P_NODE] = node_;
            eventData[P_COMPONENT] = this;

            scene->SendEvent(E_COMPONENTENABLEDCHANGED, eventData);
        }
    }
}

void Component::Remove()
{
    if (node_)
        node_->RemoveComponent(this);
}

Scene* Component::GetScene() const
{
    return node_ ? node_->GetScene() : nullptr;
}

void Component::OnNodeSet(Node* previousNode, Node* currentNode)
{
}

void Component::OnSceneSet(Scene* scene)
{
}

void Component::OnMarkedDirty(Node* node)
{
}

void Component::OnNodeSetEnabled(Node* node)
{
}

void Component::SetID(unsigned id)
{
    id_ = id;
}

void Component::SetNode(Node* node)
{
    Node* previousNode = node_;
    node_ = node;
    OnNodeSet(previousNode, node_);
}

Component* Component::GetComponent(StringHash type) const
{
    return node_ ? node_->GetComponent(type) : nullptr;
}

bool Component::IsEnabledEffective() const
{
    return enabled_ && node_ && node_->IsEnabled();
}

ea::string Component::GetFullNameDebug() const
{
    ea::string fullName = node_ ? Format("{}/({})", node_->GetFullNameDebug(), node_->GetComponentIndex(this)) : "";
    fullName += GetTypeName();
    return fullName;
}

unsigned Component::GetIndexInParent() const
{
    return node_ ? node_->GetComponentIndex(this) : M_MAX_UNSIGNED;
}

Component* Component::GetFixedUpdateSource()
{
    Component* ret = nullptr;
    Scene* scene = GetScene();

    if (scene)
    {
#ifdef URHO3D_PHYSICS
        ret = scene->GetComponent<PhysicsWorld>();
#endif
#ifdef URHO3D_PHYSICS2D
        if (!ret)
            ret = scene->GetComponent<PhysicsWorld2D>();
#endif
    }

    return ret;
}

void Component::DoAutoRemove(AutoRemoveMode mode)
{
    switch (mode)
    {
    case REMOVE_COMPONENT:
        Remove();
        return;

    case REMOVE_NODE:
        if (node_)
            node_->Remove();
        return;

    default:
        return;
    }
}

}
