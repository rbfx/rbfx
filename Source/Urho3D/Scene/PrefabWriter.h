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

#pragma once

#include <Urho3D/Scene/NodePrefab.h>

namespace Urho3D
{

/// Interface of a class that writes prefab data.
/// Useful for writing generic code in Node and Scene serialization.
/// Call order:
/// - WriteNode() should be called first exactly once.
/// - WriteNumComponents() should be called after WriteNode() exactly once.
/// - WriteComponent() should be called after WriteNumComponents() the exact number of times.
/// - WriteNumChildren() should be called after all calls to WriteComponent() exactly once.
/// - BeginChild() and then EndChild() should be called after WriteNumChildren() the exact number of times.
/// - The sequence above should be recursively repeated between BeginChild() and EndChild().
class URHO3D_API PrefabWriter : public NonCopyable
{
public:
    virtual ~PrefabWriter() = default;

    virtual void WriteNode(unsigned id, const Serializable* node) = 0;
    virtual void WriteNumComponents(unsigned numComponents) = 0;
    virtual void WriteComponent(unsigned id, const Serializable* component) = 0;
    virtual void WriteNumChildren(unsigned numChildren) = 0;
    virtual void BeginChild() = 0;
    virtual void EndChild() = 0;
    virtual bool IsEOF() const = 0;
    virtual PrefabSaveFlags GetFlags() const = 0;
};

/// Utility class to write prefab data to NodePrefab.
class URHO3D_API PrefabWriterToMemory : public PrefabWriter
{
public:
    explicit PrefabWriterToMemory(NodePrefab& nodePrefab, PrefabSaveFlags flags = {});

    void WriteNode(unsigned id, const Serializable* node) override;
    void WriteNumComponents(unsigned numComponents) override;
    void WriteComponent(unsigned id, const Serializable* component) override;
    void WriteNumChildren(unsigned numChildren) override;
    void BeginChild() override;
    void EndChild() override;
    bool IsEOF() const override { return stack_.empty(); }
    PrefabSaveFlags GetFlags() const override { return flags_; }

private:
    NodePrefab& CurrentNode() const;
    void StartChildren();
    void NextNode();
    void CheckEOF();

    NodePrefab& nodePrefab_;
    const PrefabSaveFlags flags_;

    ea::vector<ea::pair<NodePrefab*, unsigned>> stack_;
    unsigned componentIndex_{};
};

/// Utility class to write prefab data to Archive.
class URHO3D_API PrefabWriterToArchive : public PrefabWriter
{
public:
    PrefabWriterToArchive(
        Archive& archive, const char* blockName, PrefabSaveFlags saveFlags = {}, PrefabArchiveFlags archiveFlags = {});

    void WriteNode(unsigned id, const Serializable* node) override;
    void WriteNumComponents(unsigned numComponents) override;
    void WriteComponent(unsigned id, const Serializable* component) override;
    void WriteNumChildren(unsigned numChildren) override;
    void BeginChild() override;
    void EndChild() override;
    bool IsEOF() const override { return eof_; }
    PrefabSaveFlags GetFlags() const override { return saveFlags_; }

private:
    void NextSerializable();
    void CheckEOF();

    Archive& archive_;
    const PrefabArchiveFlags nodeFlags_;
    const PrefabArchiveFlags componentFlags_;
    const PrefabSaveFlags saveFlags_;
    SerializablePrefab buffer_;

    bool hasRootBlock_{};
    ea::vector<ea::pair<ArchiveBlock, unsigned>> stack_;
    bool eof_{};
};

} // namespace Urho3D
