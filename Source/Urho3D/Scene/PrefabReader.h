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

/// Interface of a class that provides prefab data.
/// Useful for writing generic code in Node and Scene serialization.
/// Call order:
/// - ReadNode() should be called first exactly once.
/// - ReadNumComponents() should be called after ReadNode() exactly once.
/// - ReadComponent() should be called after ReadNumComponents() the exact number of times.
/// - ReadNumChildren() should be called after all calls to ReadComponent() exactly once.
/// - BeginChild() and then EndChild() should be called after ReadNumChildren() the exact number of times.
/// - The sequence above should be recursively repeated between BeginChild() and EndChild().
/// - Previously returned pointers should not be used after the call to ReadNode() or ReadComponent().
class URHO3D_API PrefabReader : public NonCopyable
{
public:
    virtual ~PrefabReader() = default;

    virtual const SerializablePrefab* ReadNode() = 0;
    virtual unsigned ReadNumComponents() = 0;
    virtual const SerializablePrefab* ReadComponent() = 0;
    virtual unsigned ReadNumChildren() = 0;
    virtual void BeginChild() = 0;
    virtual void EndChild() = 0;
    virtual bool IsEOF() const = 0;
};

/// Utility class to read prefab data from NodePrefab.
class URHO3D_API PrefabReaderFromMemory : public PrefabReader
{
public:
    explicit PrefabReaderFromMemory(const NodePrefab& nodePrefab);

    const SerializablePrefab* ReadNode() override;
    unsigned ReadNumComponents() override;
    const SerializablePrefab* ReadComponent() override;
    unsigned ReadNumChildren() override;
    void BeginChild() override;
    void EndChild() override;
    bool IsEOF() const override { return stack_.empty(); }

private:
    const NodePrefab& CurrentNode() const;
    void StartChildren();
    void NextNode();
    void UpdateEOF();

    const NodePrefab& nodePrefab_;

    ea::vector<ea::pair<const NodePrefab*, unsigned>> stack_;
    unsigned componentIndex_{};
};

/// Utility class to read prefab data from Archive.
class URHO3D_API PrefabReaderFromArchive : public PrefabReader
{
public:
    PrefabReaderFromArchive(Archive& archive, const char* blockName, PrefabArchiveFlags flags = {});

    const SerializablePrefab* ReadNode() override;
    unsigned ReadNumComponents() override;
    const SerializablePrefab* ReadComponent() override;
    unsigned ReadNumChildren() override;
    void BeginChild() override;
    void EndChild() override;
    bool IsEOF() const override { return eof_; }

private:
    void NextSerializable();
    void UpdateEOF();

    Archive& archive_;
    const PrefabArchiveFlags nodeFlags_;
    const PrefabArchiveFlags componentFlags_;
    SerializablePrefab buffer_;

    bool hasRootBlock_{};
    ea::vector<ea::pair<ArchiveBlock, unsigned>> stack_;
    bool eof_{};
};

} // namespace Urho3D
