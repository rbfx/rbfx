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

#pragma once

#include "../Engine/ApplicationFlavor.h"
#include "../IO/FileSystem.h"
#include "../Scene/Serializable.h"

#include <EASTL/unordered_set.h>

namespace Urho3D
{

class AssetTransformer;
using AssetTransformerVector = ea::vector<AssetTransformer*>;

/// Transformer execution inputs (should be serializable on its own).
struct URHO3D_API AssetTransformerInput
{
    AssetTransformerInput() = default;
    AssetTransformerInput(const ApplicationFlavor& flavor, const ea::string& resourceName,
        const ea::string& inputFileName, FileTime inputFileTime);
    AssetTransformerInput(const AssetTransformerInput& other, const ea::string& tempPath,
        const ea::string& outputFileName, const ea::string& outputResourceName);

    void SerializeInBlock(Archive& archive);
    static AssetTransformerInput FromBase64(const ea::string& base64);
    ea::string ToBase64() const;

    /// Flavor of the transformer.
    ApplicationFlavor flavor_;
    /// Original resource name. May be different from resource name for nested transformers.
    ea::string originalResourceName_;
    /// Original absolute file name. May be different from file name for nested transformers.
    ea::string originalInputFileName_;

    /// Resource name that can be used to access resource via cache.
    ea::string resourceName_;
    /// Absolute file name of the asset.
    ea::string inputFileName_;
    /// Modification time of the input file.
    FileTime inputFileTime_;
    /// Absolute path to the temporary directory used to store output files.
    ea::string tempPath_;
    /// Absolute file name to the file counterpart in writeable directory.
    ea::string outputFileName_;
    /// Resource name to the file counterpart in writeable directory.
    ea::string outputResourceName_;
};

/// Transformer execution result (should be serializable on its own).
struct URHO3D_API AssetTransformerOutput
{
    void SerializeInBlock(Archive& archive);
    static AssetTransformerOutput FromBase64(const ea::string& base64);
    ea::string ToBase64() const;

    /// Whether the source file was modified.
    bool sourceModified_{};
    /// Resource names of the output files. Do not add source files here!
    ea::vector<ea::string> outputResourceNames_;
    /// Types of transformers that were applied to the asset.
    ea::unordered_set<ea::string> appliedTransformers_;
    /// Other files that were used to generate the output.
    ea::unordered_map<ea::string, FileTime> dependencyModificationTimes_;
};

/// Interface of a script that can be used to transform assets.
/// Flavor is used for hierarchical filtering.
/// Transformer with a flavor is not used unless user requested this (or more specific) flavor.
class URHO3D_API AssetTransformer : public Serializable
{
    URHO3D_OBJECT(AssetTransformer, Serializable);

public:
    explicit AssetTransformer(Context* context);

    /// Return whether the transformer array is applied to the given asset in any way.
    static bool IsApplicable(const AssetTransformerInput& input, const AssetTransformerVector& transformers);
    /// Execute transformer array on the asset.
    static bool ExecuteTransformers(const AssetTransformerInput& input, AssetTransformerOutput& output,
        const AssetTransformerVector& transformers, bool isNestedExecution);
    /// Execute transformer array on the asset and copy results in the output path.
    static bool ExecuteTransformersAndStore(const AssetTransformerInput& input, const ea::string& outputPath,
        AssetTransformerOutput& output, const AssetTransformerVector& transformers);

    /// Return whether the transformer can be applied to the given asset. Should be as fast as possible.
    virtual bool IsApplicable(const AssetTransformerInput& input) { return false; }
    /// Execute this transformer on the asset. Return true if any action was performed.
    virtual bool Execute(
        const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
    {
        return false;
    }
    /// Return whether the importer of this type should be invoked at most once.
    virtual bool IsSingleInstanced() { return true; }
    /// Return whether to execute this transformer on the output of the other transformer.
    virtual bool IsExecutedOnOutput() { return false; }

    /// Manage requirement flavor of the transformer.
    /// @{
    void SetFlavor(const ApplicationFlavorPattern& value) { flavor_ = value; }
    const ApplicationFlavorPattern& GetFlavor() const { return flavor_; }
    /// @}

protected:
    /// Helper to add file dependency.
    void AddDependency(
        const AssetTransformerInput& input, AssetTransformerOutput& output, const ea::string& fileName) const;

private:
    ApplicationFlavorPattern flavor_{};
};

/// Dependency between two transformer classes.
/// Note: this dependency is global within a project regardless of transformer scope!
struct AssetTransformerDependency
{
    ea::string class_;
    ea::string dependsOn_;

    void SerializeInBlock(Archive& archive);
};

} // namespace Urho3D
