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

#include <Urho3D/Precompiled.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>

#include <EASTL/unordered_set.h>

namespace Urho3D
{

PrefabResource::PrefabResource(Context* context)
    : SimpleResource(context)
{
}

PrefabResource::~PrefabResource()
{
}

void PrefabResource::RegisterObject(Context* context)
{
    context->AddFactoryReflection<PrefabResource>();
}

void PrefabResource::NormalizeIds()
{
    prefab_.NormalizeIds(context_);

    auto& sceneAttributes = prefab_.GetMutableNode().GetMutableAttributes();
    static const ea::unordered_set<StringHash> idAttributes{"Next Node ID", "Next Component ID"};
    const auto isIdAttribute = [](const AttributePrefab& attr) { return idAttributes.contains(attr.GetNameHash()); };
    ea::erase_if(sceneAttributes, isIdAttribute);
}

void PrefabResource::SerializeInBlock(Archive& archive)
{
    // For prefabs, we keep as much information as possible, because prefabs shouldn't be too heavy.
    // Can always turn on "compactSave" later.
    const bool compactSave = false;
    const auto flags = PrefabArchiveFlag::None;

    prefab_.SerializeInBlock(archive, flags, compactSave);
}

const NodePrefab& PrefabResource::GetNodePrefab() const
{
    return !prefab_.GetChildren().empty() ? prefab_.GetChildren()[0] : NodePrefab::Empty;
}

NodePrefab& PrefabResource::GetMutableNodePrefab()
{
    auto& children = prefab_.GetMutableChildren();
    if (children.empty())
        children.emplace_back();
    return children[0];
}

bool PrefabResource::LoadLegacyXML(const XMLElement& source)
{
    if (source.GetName() != "scene")
        return false;

    // This is awful, but we cannot do better because old prefab format has incomplete information.
    auto tempScene = MakeShared<Scene>(context_);
    if (!tempScene->LoadXML(source))
        return false;

    tempScene->GeneratePrefab(prefab_);

    static const char* helpMessage =
        "To convert prefab into new format:\n"
        "1. Rename file to *.prefab;\n"
        "2. Open it in the Editor as Scene (LMB double click on resource);\n"
        "3. Save it normally ('Save' icon or menu item).\n";

    URHO3D_LOGWARNING("Legacy prefab format is used in file '{}'! {}", GetName(), helpMessage);
    return true;
}

} // namespace Urho3D
