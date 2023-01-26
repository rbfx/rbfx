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

#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/PrefabWriter.h>

namespace Urho3D
{

PrefabWriterToMemory::PrefabWriterToMemory(NodePrefab& nodePrefab, PrefabSaveFlags flags)
    : nodePrefab_(nodePrefab)
    , flags_(flags)
    , stack_{{nullptr, 0u}}
{
    nodePrefab_.Clear();
}

void PrefabWriterToMemory::WriteNode(unsigned id, const Serializable* node)
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to write");

    SerializablePrefab& nodePrefab = CurrentNode().GetMutableNode();
    nodePrefab.SetId(static_cast<SerializableId>(id));
    nodePrefab.Import(node, flags_);
    nodePrefab.SetType(StringHash::Empty);
}

void PrefabWriterToMemory::WriteNumComponents(unsigned numComponents)
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to write");

    CurrentNode().GetMutableComponents().resize(numComponents);
    componentIndex_ = 0;
}

void PrefabWriterToMemory::WriteComponent(unsigned id, const Serializable* component)
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to write");

    auto& components = CurrentNode().GetMutableComponents();
    URHO3D_ASSERT(componentIndex_ < components.size());

    auto& componentPrefab = components[componentIndex_];
    componentPrefab.SetId(static_cast<SerializableId>(id));
    componentPrefab.Import(component, flags_);

    ++componentIndex_;
}

void PrefabWriterToMemory::WriteNumChildren(unsigned numChildren)
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to write");

    if (numChildren > 0)
    {
        CurrentNode().GetMutableChildren().resize(numChildren);
        StartChildren();
    }
    else
        CheckEOF();
}

void PrefabWriterToMemory::BeginChild()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to write");
}

void PrefabWriterToMemory::EndChild()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to write");

    NextNode();
    CheckEOF();
}

NodePrefab& PrefabWriterToMemory::CurrentNode() const
{
    auto& [parentNode, childIndex] = stack_.back();
    return parentNode ? parentNode->GetMutableChildren()[childIndex] : nodePrefab_;
}

void PrefabWriterToMemory::StartChildren()
{
    stack_.emplace_back(&CurrentNode(), 0u);
}

void PrefabWriterToMemory::NextNode()
{
    auto& [parentNode, childIndex] = stack_.back();

    const unsigned maxChildren = parentNode ? parentNode->GetChildren().size() : 1u;
    ++childIndex;
    if (childIndex >= maxChildren)
        stack_.pop_back();
}

void PrefabWriterToMemory::CheckEOF()
{
    if (stack_.size() == 1)
        stack_.clear();
}

PrefabWriterToArchive::PrefabWriterToArchive(
    Archive& archive, const char* blockName, PrefabSaveFlags saveFlags, PrefabArchiveFlags archiveFlags)
    : archive_(archive)
    , nodeFlags_(ToNodeFlags(archiveFlags))
    , componentFlags_(ToComponentFlags(archiveFlags))
    , saveFlags_(saveFlags)
{
    if (blockName != nullptr)
    {
        auto rootBlock = archive_.OpenUnorderedBlock(blockName);
        stack_.emplace_back(ea::move(rootBlock), 1u);
        hasRootBlock_ = true;
    }
    URHO3D_ASSERT(!archive.IsInput());
}

void PrefabWriterToArchive::WriteNode(unsigned id, const Serializable* node)
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to write");

    buffer_.SetId(static_cast<SerializableId>(id));
    buffer_.Import(node, saveFlags_);
    buffer_.SerializeInBlock(archive_, nodeFlags_);
}

void PrefabWriterToArchive::WriteNumComponents(unsigned numComponents)
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to write");

    SerializeOptionalValue(archive_, "components", numComponents, 0u,
        [&](Archive& archive, const char* name, unsigned&)
    {
        auto block = archive_.OpenArrayBlock(name, numComponents);
        if (numComponents > 0)
            stack_.emplace_back(ea::move(block), numComponents);
    });
}

void PrefabWriterToArchive::WriteComponent(unsigned id, const Serializable* component)
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to write");

    buffer_.SetId(static_cast<SerializableId>(id));
    buffer_.Import(component, saveFlags_);

    {
        ArchiveBlock block = archive_.OpenUnorderedBlock("component");
        buffer_.SerializeInBlock(archive_, componentFlags_);
    }

    NextSerializable();
}

void PrefabWriterToArchive::WriteNumChildren(unsigned numChildren)
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to write");

    SerializeOptionalValue(archive_, "nodes", numChildren, 0u,
        [&](Archive& archive, const char* name, unsigned&)
    {
        auto block = archive_.OpenArrayBlock(name, numChildren);
        if (numChildren > 0)
            stack_.emplace_back(ea::move(block), numChildren);
        else
            CheckEOF();
    });
}

void PrefabWriterToArchive::BeginChild()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to write");

    ArchiveBlock block = archive_.OpenUnorderedBlock("node");
    stack_.emplace_back(ea::move(block), 1u);
}

void PrefabWriterToArchive::EndChild()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to write");

    stack_.pop_back();
    NextSerializable();
    CheckEOF();
}

void PrefabWriterToArchive::NextSerializable()
{
    if (!stack_.empty())
    {
        --stack_.back().second;
        if (stack_.back().second == 0)
            stack_.pop_back();
    }
}

void PrefabWriterToArchive::CheckEOF()
{
    if (stack_.size() == (hasRootBlock_ ? 1 : 0))
    {
        stack_.clear();
        eof_ = true;
    }
}

} // namespace Urho3D
