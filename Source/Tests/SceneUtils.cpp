// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "SceneUtils.h"

#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/PrefabWriter.h>

namespace Tests
{

void SerializeAndDeserializeScene(Scene* scene)
{
    auto xmlFile = MakeShared<XMLFile>(scene->GetContext());
    auto xmlRoot = xmlFile->GetOrCreateRoot("scene");
    scene->SaveXML(xmlRoot);
    scene->Clear();
    scene->LoadXML(xmlRoot);
}

SharedPtr<PrefabResource> ConvertNodeToPrefab(Node* node)
{
    auto prefab = MakeShared<PrefabResource>(node->GetContext());
    prefab->GetMutableNodePrefab() = node->GeneratePrefab();
    prefab->NormalizeIds();
    return prefab;
}

Variant GetAttributeValue(const ea::pair<Serializable*, unsigned>& ref)
{
    return ref.first->GetAttribute(ref.second);
}

bool CompareAttributeValues(const Variant& lhs, const Variant& rhs)
{
    return lhs == rhs;
}

bool CompareSerializables(const Serializable& lhs, const Serializable& rhs)
{
    const auto lhsAttributes = lhs.GetAttributes();
    const auto rhsAttributes = rhs.GetAttributes();

    if (!lhsAttributes || !rhsAttributes)
        return !lhsAttributes && !rhsAttributes;

    if (lhsAttributes->size() != rhsAttributes->size())
        return false;

    for (unsigned i = 0; i < lhsAttributes->size(); ++i)
    {
        if (!CompareAttributeValues(lhs.GetAttribute(i), lhs.GetAttribute(i)))
            return false;
    }

    return true;
}

bool CompareNodes(const Node& lhs, const Node& rhs)
{
    if (!CompareSerializables(lhs, rhs))
        return false;

    const auto& lhsComponents = lhs.GetComponents();
    const auto& rhsComponents = rhs.GetComponents();
    const bool sameComponents = ea::identical(
        lhsComponents.begin(), lhsComponents.end(), rhsComponents.begin(), rhsComponents.end(),
        [](const auto& lhs, const auto& rhs) { return CompareSerializables(*lhs, *rhs); });

    const auto& lhsChildren = lhs.GetChildren();
    const auto& rhsChildren = rhs.GetChildren();
    const bool sameChildren = ea::identical(
        lhsChildren.begin(), lhsChildren.end(), rhsChildren.begin(), rhsChildren.end(),
        [](const auto& lhs, const auto& rhs) { return CompareNodes(*lhs, *rhs); });

    return sameComponents && sameChildren;
}

Node* NodeRef::GetNode() const
{
    return scene_ ? scene_->GetChild(name_, true) : nullptr;
}

}
