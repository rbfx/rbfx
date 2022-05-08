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

#include "../Resource/Resource.h"
#include "../Core/PatternMatching.h"

namespace Urho3D
{
class Model;

/// Character configuration resource.
class URHO3D_API CharacterConfiguration : public Resource
{
    URHO3D_OBJECT(CharacterConfiguration, Resource)
private:
    struct BodyPart
    {
        /// Serialize from/to archive. Return true if successful.
        void SerializeInBlock(Archive& archive);

        /// State machine via fuzzy pattern matching.
        ea::string attachmentBone_;
        /// Model selector via fuzzy pattern matching.
        PatternCollection modelSelector_;
    };

public:
    /// Construct.
    explicit CharacterConfiguration(Context* context);
    /// Destruct.
    ~CharacterConfiguration() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;

    /// Save resource. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Serialize from/to archive. Return true if successful.
    void SerializeInBlock(Archive& archive) override;

    /// Resize body parts vector.
    void SetNumBodyParts(unsigned num);
    /// Get number of body parts.
    unsigned GetNumBodyParts() const;

    //void SetModel(Model* model);
    void SetModelAttr(ResourceRef model);
    ResourceRef GetModelAttr() const { return skeletonModel_; }
    void SetMaterialsAttr(const ResourceRefList& materials);
    const ResourceRefList& GetMaterialsAttr() const { return skeletonMaterials_; }

private:
    /// Reset to defaults.
    void ResetToDefaults();

    /// Skeleton model that has complete bone structure 
    ResourceRef skeletonModel_;

    /// Skeleton model materials
    ResourceRefList skeletonMaterials_;

    /// Character body parts.
    std::vector<BodyPart> bodyParts_;

    /// State machine via fuzzy pattern matching.
    PatternCollection stateMachine_;
};

}
