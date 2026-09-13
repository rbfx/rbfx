// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Scene/NodePrefab.h"

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
