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
#include <Urho3D/Scene/PrefabReader.h>

namespace Urho3D
{

PrefabReaderFromMemory::PrefabReaderFromMemory(const NodePrefab& nodePrefab)
    : nodePrefab_(nodePrefab)
    , stack_{{nullptr, 0u}}
{
}

const SerializablePrefab* PrefabReaderFromMemory::ReadNode()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to read");

    return &CurrentNode().GetNode();
}

unsigned PrefabReaderFromMemory::ReadNumComponents()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to read");

    componentIndex_ = 0;
    return CurrentNode().GetComponents().size();
}

const SerializablePrefab* PrefabReaderFromMemory::ReadComponent()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to read");

    const auto& components = CurrentNode().GetComponents();
    URHO3D_ASSERT(componentIndex_ < components.size());

    const SerializablePrefab& component = components[componentIndex_];
    ++componentIndex_;
    return &component;
}

unsigned PrefabReaderFromMemory::ReadNumChildren()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to read");

    const unsigned numChildren = CurrentNode().GetChildren().size();
    if (numChildren > 0)
        StartChildren();
    else
        UpdateEOF();
    return numChildren;
}

void PrefabReaderFromMemory::BeginChild()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to read");
}

void PrefabReaderFromMemory::EndChild()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to read");

    NextNode();
    UpdateEOF();
}

const NodePrefab& PrefabReaderFromMemory::CurrentNode() const
{
    const auto& [parentNode, childIndex] = stack_.back();
    return parentNode ? parentNode->GetChildren()[childIndex] : nodePrefab_;
}

void PrefabReaderFromMemory::StartChildren()
{
    stack_.emplace_back(&CurrentNode(), 0u);
}

void PrefabReaderFromMemory::NextNode()
{
    auto& [parentNode, childIndex] = stack_.back();

    const unsigned maxChildren = parentNode ? parentNode->GetChildren().size() : 1u;
    ++childIndex;
    if (childIndex >= maxChildren)
        stack_.pop_back();
}

void PrefabReaderFromMemory::UpdateEOF()
{
    if (stack_.size() == 1)
        stack_.clear();
}

PrefabReaderFromArchive::PrefabReaderFromArchive(
    Archive& archive, const char* blockName, PrefabArchiveFlags flags)
    : archive_(archive)
    , nodeFlags_(ToNodeFlags(flags))
    , componentFlags_(ToComponentFlags(flags))
{
    if (blockName != nullptr)
    {
        auto rootBlock = archive_.OpenUnorderedBlock(blockName);
        stack_.emplace_back(ea::move(rootBlock), 1u);
        hasRootBlock_ = true;
    }
    URHO3D_ASSERT(archive.IsInput());
}

void PrefabReaderFromArchive::NextSerializable()
{
    if (!stack_.empty())
    {
        --stack_.back().second;
        if (stack_.back().second == 0)
            stack_.pop_back();
    }
}

const SerializablePrefab* PrefabReaderFromArchive::ReadNode()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to read");

    buffer_.SerializeInBlock(archive_, nodeFlags_);

    return &buffer_;
}

unsigned PrefabReaderFromArchive::ReadNumComponents()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to read");

    int placeholder = 0;
    unsigned numComponents = 0;
    SerializeOptionalValue(archive_, "components", placeholder, {},
        [&](Archive& archive, const char* name, int&)
    {
        auto block = archive_.OpenArrayBlock(name);
        numComponents = block.GetSizeHint();
        if (numComponents > 0)
            stack_.emplace_back(ea::move(block), numComponents);
    });
    return numComponents;
}

const SerializablePrefab* PrefabReaderFromArchive::ReadComponent()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to read");

    {
        ArchiveBlock block = archive_.OpenUnorderedBlock("component");
        buffer_.SerializeInBlock(archive_, componentFlags_);
    }
    NextSerializable();

    return &buffer_;
}

unsigned PrefabReaderFromArchive::ReadNumChildren()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to read");

    int placeholder = 0;
    unsigned numChildren = 0;
    SerializeOptionalValue(archive_, "nodes", placeholder, {},
        [&](Archive& archive, const char* name, int&)
    {
        auto block = archive_.OpenArrayBlock(name);
        numChildren = block.GetSizeHint();
        if (numChildren > 0)
            stack_.emplace_back(ea::move(block), numChildren);
        else
            UpdateEOF();
    });
    return numChildren;
}

void PrefabReaderFromArchive::BeginChild()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to read");

    ArchiveBlock block = archive_.OpenUnorderedBlock("node");
    stack_.emplace_back(ea::move(block), 1u);
}

void PrefabReaderFromArchive::EndChild()
{
    URHO3D_ASSERT(!IsEOF(), "There is no more data to read");

    stack_.pop_back();
    NextSerializable();
    UpdateEOF();
}

void PrefabReaderFromArchive::UpdateEOF()
{
    if (stack_.size() == (hasRootBlock_ ? 1 : 0))
    {
        stack_.clear();
        eof_ = true;
    }
}

} // namespace Urho3D
