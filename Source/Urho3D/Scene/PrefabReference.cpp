//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../Scene/PrefabReference.h"

#include "Node.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/ResourceEvents.h"

namespace Urho3D
{

namespace
{
void SetTemporaryFlag(Node* node, bool isTemporary)
{
    if (!node)
    {
        return;
    }

    node->SetTemporary(isTemporary);

    ea::vector<PrefabReference*> refs;
    node->GetComponents<PrefabReference>(refs, false);

    for (auto& component : node->GetComponents())
    {
        component->SetTemporary(isTemporary);
    }

    for (auto& child : node->GetChildren())
    {
        bool isPrefabRoot = false;
        for (auto* prefabRef: refs)
        {
            //Skip inner prefabs
            if (child == prefabRef->GetRootNode())
            {
                isPrefabRoot = true;
                break;;
            }
        }
        if (!isPrefabRoot)
        {
            SetTemporaryFlag(child, isTemporary);
        }
    }
}
} // namespace

PrefabReference::PrefabReference(Context* context)
    : BaseClassName(context)
    , prefabRef_(XMLFile::GetTypeStatic())
{
}

PrefabReference::~PrefabReference() = default;

void PrefabReference::RegisterObject(Context* context)
{
    context->RegisterFactory<PrefabReference>(Category_Scene);

    URHO3D_ACTION_STATIC_LABEL("Inline", Inline, "Inline the prefab nodes");

    URHO3D_ACCESSOR_ATTRIBUTE("Preserve Transform", GetPreserveTransfrom, SetPreserveTransfrom, bool, false, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Prefab", GetPrefabAttr, SetPrefabAttr, ResourceRef, ResourceRef(XMLFile::GetTypeStatic()), AM_DEFAULT);
}

/// Set prefab resource.
void PrefabReference::SetPrefab(XMLFile* prefab)
{
    if (prefab == prefab_)
    {
        return;
    }

    if (prefab_)
    {
        UnsubscribeFromEvent(prefab_, E_RELOADFINISHED);
    }

    prefab_ = prefab;

    if (prefab_)
    {
        SubscribeToEvent(prefab_, E_RELOADFINISHED, URHO3D_HANDLER(PrefabReference, HandlePrefabReloaded));
        prefabRef_ = GetResourceRef(prefab_, XMLFile::GetTypeStatic());
    }
    else
    {
        prefabRef_ = ResourceRef(XMLFile::GetTypeStatic());
    }

    ToggleNode(true);
}

/// Get prefab resource.
XMLFile* PrefabReference::GetPrefab() const
{
    return prefab_;
}

/// Set flag to preserve prefab root node transform.
void PrefabReference::SetPreserveTransfrom(bool preserve)
{
    if (preserve != preserveTransform_)
    {
        preserveTransform_ = preserve;
        if (node_)
        {
            node_->Remove();
            node_ = CreateInstance();
        }
    }
}


/// Set reference to prefab resource.
void PrefabReference::SetPrefabAttr(ResourceRef prefab)
{
    if (prefab.name_.empty())
    {
        SetPrefab(nullptr);
    }
    else
    {
        SetPrefab(context_->GetSubsystem<ResourceCache>()->GetResource<XMLFile>(prefab.name_));
    }
    prefabRef_ = prefab;
}

/// Get reference to prefab resource.
ResourceRef PrefabReference::GetPrefabAttr() const { return prefabRef_; }


/// Create prefab instance.
Node* PrefabReference::CreateInstance() const
{
    if (!prefab_)
    {
        return nullptr;
    }

    auto* node = GetNode()->CreateTemporaryChild();

    // If we parsing a root file element and it is scene then parse the first node of the scene.
    XMLElement rootElement = prefab_->GetRoot();
    if (!rootElement)
    {
        return nullptr;
    }

    if (rootElement.GetName() == "scene")
    {
        const XMLElement firstNode = rootElement.GetChild("node");
        const XMLElement nextNode = firstNode.GetNext("node");
        if (!nextNode.IsNull())
        {
            URHO3D_LOGERROR("More than one root node in prefab");
        }
        rootElement = firstNode;
    }

    node->LoadXML(rootElement);

    if (!preserveTransform_)
    {
        node->SetPosition(Vector3::ZERO);
        node->SetRotation(Quaternion::IDENTITY);
        node->SetScale(Vector3::ONE);
    }
    SetTemporaryFlag(node, true);
    return node;
}

void PrefabReference::Inline()
{
    SetTemporaryFlag(node_, false);
    node_.Reset();
    Remove();
}

/// Handle enabled/disabled state change.
void PrefabReference::OnSetEnabled()
{
    ToggleNode();
}

/// Handle scene node being assigned at creation.
void PrefabReference::OnNodeSet(Node* node)
{
    ToggleNode();
}

/// Create or remove prefab based on current environment.
void PrefabReference::ToggleNode(bool forceReload)
{
    Node* node = GetNode();
    const bool enabled = node?IsEnabledEffective():false;

    if (!enabled)
    {
        if (node_ && node_->GetParent())
        {
            // Don't destroy the node as it may be enabled later.
            node_->Remove();

            // If reload is required then unload existing node so it will be reloaded when required.
            if (forceReload)
                node_.Reset();
        }
    }
    else
    {
        // Reload node if necessary.
        if (forceReload || !node_)
        {
            if (node_)
            {
                node_->Remove();
                node_.Reset();
            }
            node_ = CreateInstance();
        }

        // Add it to the component's parent.
        if (node_ && node != node_->GetParent())
        {
            node_->Remove();
            node->AddChild(node_);
        }
    }
}

/// Get prefab instance root node.
Node* PrefabReference::GetRootNode() const
{
    return node_;
}

void PrefabReference::HandlePrefabReloaded(StringHash eventType, VariantMap& map)
{
    ToggleNode(true);
}

}
