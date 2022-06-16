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
extern const char* SCENE_CATEGORY;

namespace
{
void MarkAsTemp(Node* node)
{
    node->SetTemporary(true);
    for (auto& component : node->GetComponents())
    {
        component->SetTemporary(true);
    }

    for (auto& child : node->GetChildren())
    {
        MarkAsTemp(child);
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
    context->RegisterFactory<PrefabReference>(SCENE_CATEGORY);
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

    auto* node = new Node(context_);
    node->LoadXML(prefab_->GetRoot());
    MarkAsTemp(node);
    return node;
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
            }
            node_ = CreateInstance();
        }

        // Add it to the component's parent.
        if (node_ && node != node_->GetParent())
        {
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
