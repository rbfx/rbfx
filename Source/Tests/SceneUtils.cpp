//
// Copyright (c) 2017-2021 the rbfx project.
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

#pragma once

#include "SceneUtils.h"

#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Component.h>

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
