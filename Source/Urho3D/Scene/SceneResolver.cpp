// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IO/Log.h"
#include "Urho3D/Scene/Component.h"
#include "Urho3D/Scene/SceneResolver.h"
#include "Urho3D/Scene/Node.h"

#include <EASTL/unordered_set.h>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

SceneResolver::SceneResolver() = default;

SceneResolver::~SceneResolver() = default;

void SceneResolver::Reset()
{
    nodeLookup_.clear();
    componentLookup_.clear();
}

void SceneResolver::AddNode(unsigned oldID, Node* node)
{
    if (node)
        nodeLookup_[oldID] = node;
}

void SceneResolver::AddComponent(unsigned oldID, Component* component)
{
    if (component)
    {
        componentLookup_[oldID] = component;
        components_.emplace_back(component);
    }
}

void SceneResolver::Resolve()
{
    // Nodes do not have component or node ID attributes, so only have to go through components
    ea::hash_set<StringHash> noIDAttributes;
    for (auto i = components_.begin(); i != components_.end(); ++i)
    {
        Component* component = *i;
        if (!component || noIDAttributes.contains(component->GetType()))
            continue;

        bool hasIDAttributes = false;
        const ea::vector<AttributeInfo>* attributes = component->GetAttributes();
        if (!attributes)
        {
            noIDAttributes.insert(component->GetType());
            continue;
        }

        for (unsigned j = 0; j < attributes->size(); ++j)
        {
            const AttributeInfo& info = attributes->at(j);
            if (info.mode_ & AM_NODEID)
            {
                hasIDAttributes = true;
                unsigned oldNodeID = component->GetAttribute(j).GetUInt();

                if (oldNodeID)
                {
                    auto k = nodeLookup_.find(oldNodeID);

                    if (k != nodeLookup_.end() && k->second)
                    {
                        unsigned newNodeID = k->second->GetID();
                        component->SetAttribute(j, Variant(newNodeID));
                    }
                    else
                        URHO3D_LOGWARNING("Could not resolve node ID " + ea::to_string(oldNodeID));
                }
            }
            else if (info.mode_ & AM_COMPONENTID)
            {
                hasIDAttributes = true;
                unsigned oldComponentID = component->GetAttribute(j).GetUInt();

                if (oldComponentID)
                {
                    auto k = componentLookup_.find(
                        oldComponentID);

                    if (k != componentLookup_.end() && k->second)
                    {
                        unsigned newComponentID = k->second->GetID();
                        component->SetAttribute(j, Variant(newComponentID));
                    }
                    else
                        URHO3D_LOGWARNING("Could not resolve component ID " + ea::to_string(oldComponentID));
                }
            }
            else if (info.mode_ & AM_NODEIDVECTOR)
            {
                hasIDAttributes = true;
                Variant attrValue = component->GetAttribute(j);
                const VariantVector& oldNodeIDs = attrValue.GetVariantVector();

                if (oldNodeIDs.size())
                {
                    // The first index stores the number of IDs redundantly. This is for editing
                    unsigned numIDs = oldNodeIDs[0].GetUInt();
                    VariantVector newIDs;
                    newIDs.push_back(numIDs);

                    for (unsigned k = 1; k < oldNodeIDs.size(); ++k)
                    {
                        unsigned oldNodeID = oldNodeIDs[k].GetUInt();
                        auto l = nodeLookup_.find(oldNodeID);

                        if (l != nodeLookup_.end() && l->second)
                            newIDs.push_back(l->second->GetID());
                        else
                        {
                            // If node was not found, retain number of elements, just store ID 0
                            newIDs.push_back(0);
                            URHO3D_LOGWARNING("Could not resolve node ID " + ea::to_string(oldNodeID));
                        }
                    }

                    component->SetAttribute(j, newIDs);
                }
            }
        }

        // If component type had no ID attributes, cache this fact for optimization
        if (!hasIDAttributes)
            noIDAttributes.insert(component->GetType());
    }

    // Attributes have been resolved, so no need to remember the nodes after this
    Reset();
}

}
