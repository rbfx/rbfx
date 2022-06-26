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

#include "../Scene/Serializable.h"

namespace Urho3D
{

class AssetTransformer;
using AssetTransformerVector = ea::vector<AssetTransformer*>;

/// Transformer execution inputs (should be serializable on its own).
struct URHO3D_API AssetTransformerInput
{
    AssetTransformerInput() = default;
    AssetTransformerInput(const ea::string& flavor, const ea::string& resourceName, const ea::string& inputFileName);
    AssetTransformerInput(const AssetTransformerInput& other, const ea::string& outputFileName);

    void SerializeInBlock(Archive& archive);
    static AssetTransformerInput FromBase64(const ea::string& base64);
    ea::string ToBase64() const;

    /// Flavor of the transformer.
    ea::string flavor_;
    /// Resource name that can be used to access resource via cache.
    ea::string resourceName_;
    /// Absolute file name to the processed resource.
    ea::string fileName_;
    /// Absolute file name to the file counterpart in writeable directory.
    ea::string outputFileName_;
};

/// Transformer execution result (should be serializable on its own).
struct URHO3D_API AssetTransformerOutput
{
    void SerializeInBlock(Archive& archive);
    static AssetTransformerOutput FromBase64(const ea::string& base64);
    ea::string ToBase64() const;

    /// Transformers that were applied to the asset, referenced by index.
    ea::vector<unsigned> appliedTransformers_;
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
    static bool Execute(const AssetTransformerInput& input, const AssetTransformerVector& transformers, AssetTransformerOutput& output);

    /// Return whether the transformer can be applied to the given asset. Should be as fast as possible.
    virtual bool IsApplicable(const AssetTransformerInput& input) { return false; }
    /// Execute this transformer on the asset. Return true if any action was performed.
    virtual bool Execute(const AssetTransformerInput& input, AssetTransformerOutput& output) { return false; }
    /// Return whether the importer of this type should be invoked at most once.
    virtual bool IsSingleInstanced() { return true; }

    /// Manage requirement flavor of the transformer.
    /// @{
    void SetFlavor(const ea::string& value);
    const ea::string& GetFlavor() const { return flavor_; }
    /// @}

private:
    ea::string flavor_{"*"};
};

}
