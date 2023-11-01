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

#include "../IO/BinaryArchive.h"
#include "../IO/MemoryBuffer.h"
#include "../Scene/Component.h"
#include "../Scene/Scene.h"
#include "../Utility/PackedSceneData.h"

#include "../DebugNew.h"

namespace Urho3D
{

PackedNodeData::PackedNodeData(Node* node)
    : id_(node->GetID())
    , parentId_(node->GetParent() ? node->GetParent()->GetID() : 0)
    , indexInParent_(node->GetIndexInParent())
    , name_(node->GetName())
{
    ConsumeArchiveException([&]
    {
        BinaryOutputArchive archive{node->GetContext(), data_};

        ArchiveBlock block = archive.OpenUnorderedBlock("node");
        node->SerializeInBlock(archive, true /* serialize temporary */, PrefabSaveFlag::CompactAttributeNames);
        scopeHint_ = node->GetEffectiveScopeHint();
    });
}

Node* PackedNodeData::SpawnExact(Scene* scene) const
{
    Node* parent = scene->GetNode(parentId_);
    if (parent == nullptr)
        return nullptr;

    Node* node = parent->CreateChild(name_, id_);
    if (node->GetID() != id_)
    {
        node->Remove();
        return nullptr;
    }

    ConsumeArchiveException([&]
    {
        MemoryBuffer view{data_.GetBuffer()};
        BinaryInputArchive archive{scene->GetContext(), view};

        ArchiveBlock block = archive.OpenUnorderedBlock("node");
        node->SerializeInBlock(archive, true /* serialize temporary */, PrefabSaveFlag::CompactAttributeNames);
    });

    parent->ReorderChild(node, indexInParent_);
    return node;
}

Node* PackedNodeData::SpawnCopy(Node* parent) const
{
    Node* node = parent->CreateChild(name_);

    ConsumeArchiveException([&]
    {
        MemoryBuffer view{data_.GetBuffer()};
        BinaryInputArchive archive{parent->GetContext(), view};

        ArchiveBlock block = archive.OpenUnorderedBlock("node");
        node->SerializeInBlock(archive, true /* serialize temporary */, PrefabSaveFlag::CompactAttributeNames);
    });

    return node;
}

PackedComponentData::PackedComponentData(Component* component)
    : id_(component->GetID())
    , nodeId_(component->GetNode() ? component->GetNode()->GetID() : 0)
    , indexInParent_(component->GetIndexInParent())
    , type_(component->GetType())
{
    ConsumeArchiveException([&]
    {
        BinaryOutputArchive archive{component->GetContext(), data_};

        ArchiveBlock block = archive.OpenUnorderedBlock("component");
        component->SerializeInBlock(archive, true /* serialize temporary */);
    });
}

Component* PackedComponentData::SpawnExact(Scene* scene) const
{
    Node* node = scene->GetNode(nodeId_);
    if (node == nullptr)
        return nullptr;

    Component* component = node->CreateComponent(type_, id_);
    if (component == nullptr)
        return nullptr;

    if (component->GetID() != id_)
    {
        component->Remove();
        return nullptr;
    }

    ConsumeArchiveException([&]
    {
        MemoryBuffer view{data_.GetBuffer()};
        BinaryInputArchive archive{scene->GetContext(), view};

        ArchiveBlock block = archive.OpenUnorderedBlock("component");
        component->SerializeInBlock(archive, true /* serialize temporary */);
    });

    node->ReorderComponent(component, indexInParent_);
    return component;
}

Component* PackedComponentData::SpawnCopy(Node* node) const
{
    Component* component = node->CreateComponent(type_, id_);
    if (component == nullptr)
        return nullptr;

    ConsumeArchiveException([&]
    {
        MemoryBuffer view{data_.GetBuffer()};
        BinaryInputArchive archive{node->GetContext(), view};

        ArchiveBlock block = archive.OpenUnorderedBlock("component");
        component->SerializeInBlock(archive, true /* serialize temporary */);
    });

    return component;
}

void PackedComponentData::Update(Component* component) const
{
    ConsumeArchiveException([&]
    {
        MemoryBuffer view{data_.GetBuffer()};
        BinaryInputArchive archive{component->GetContext(), view};

        ArchiveBlock block = archive.OpenUnorderedBlock("component");
        component->SerializeInBlock(archive, true /* serialize temporary */);
    });
}

void PackedSceneData::ToScene(Scene* scene) const
{
    ConsumeArchiveException([&]
    {
        MemoryBuffer view{sceneData_.GetBuffer()};
        BinaryInputArchive archive{scene->GetContext(), view};

        ArchiveBlock block = archive.OpenUnorderedBlock("scene");
        scene->SerializeInBlock(archive, true /* serialize temporary */, PrefabSaveFlag::CompactAttributeNames);
    });
}

PackedSceneData PackedSceneData::FromScene(Scene* scene)
{
    PackedSceneData result;

    ConsumeArchiveException([&]
    {
        BinaryOutputArchive archive{scene->GetContext(), result.sceneData_};

        ArchiveBlock block = archive.OpenUnorderedBlock("scene");
        scene->SerializeInBlock(archive, true /* serialize temporary */, PrefabSaveFlag::CompactAttributeNames);
    });

    return result;
}

}
