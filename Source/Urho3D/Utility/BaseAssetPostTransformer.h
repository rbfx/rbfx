// Copyright (c) 2020-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/Utility/AssetTransformer.h"

namespace Urho3D
{

/// Base class for post-transformers.
class URHO3D_API BaseAssetPostTransformer : public AssetTransformer
{
public:
    using AssetTransformer::AssetTransformer;

    /// File name of parameter file connected to this transformer.
    virtual ea::string_view GetParametersFileName() const = 0;

    /// Implement AssetTransformer.
    /// @{
    bool IsApplicable(const AssetTransformerInput& input) override;
    bool IsPostTransform() override { return true; }
    /// @}

protected:
    /// Load parameters from file.
    template <class T> T LoadParameters(const ea::string& fileName) const
    {
        T result;

        JSONFile file{context_};
        if (file.LoadFile(fileName))
        {
            if (file.LoadObject("params", result))
                return result;
        }

        return {};
    }

    struct PatternMatch
    {
        ea::string fileName_;
        ea::string match_;
    };

    /// Return files in resource directory that match the template.
    ea::vector<PatternMatch> GetResourcesByPattern(
        const ea::string& baseResourceName, const ea::string& fileNamePattern) const;
    /// Return file name for template match.
    ea::string GetMatchFileName(const ea::string& fileNameTemplate, const PatternMatch& match) const;
};

} // namespace Urho3D
