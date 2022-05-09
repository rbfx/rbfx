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
class Material;

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

    void SetModel(Model* model);
    void SetModelAttr(ResourceRef model);
    ResourceRef GetModelAttr() const { return model_; }

    void SetMaterial(Material* material);
    void SetMaterialAttr(const ResourceRefList& materials);
    const ResourceRefList& GetMaterialAttr() const { return material_; }

    /// Set shadowcaster flag.
    /// @property
    void SetCastShadows(bool enable);

    /// Return shadowcaster flag.
    /// @property
    bool GetCastShadows() const { return castShadows_; }

    /// Set position in parent space.
    /// @property
    void SetPosition(const Vector3& position);
    /// Set rotation in parent space.
    /// @property
    void SetRotation(const Quaternion& rotation);
    /// Set uniform scale in parent space.
    void SetScale(float scale);
    /// Set scale in parent space.
    /// @property
    void SetScale(const Vector3& scale);
    /// Return position in parent space.
    /// @property
    const Vector3& GetPosition() const { return position_; }
    /// Return rotation in parent space.
    /// @property
    const Quaternion& GetRotation() const { return rotation_; }
    /// Return scale in parent space.
    /// @property
    const Vector3& GetScale() const { return scale_; }

    PatternCollection* GetStates() { return &stateMachine_; }

    const Matrix3x4& LocalToWorld() { return localToWorld_; }
    const Matrix3x4& WorldToLocal() { return worldToLocal_; }

private:
    /// Reset to defaults.
    void ResetToDefaults();
    /// Update matrices.
    void UpdateMatrices();

    /// Skeleton model that has complete bone structure 
    ResourceRef model_;
    /// Skeleton model materials
    ResourceRefList material_;
    Vector3 position_ {Vector3::ZERO};
    Quaternion rotation_{Quaternion::IDENTITY};
    Vector3 scale_{Vector3::ONE};
    bool castShadows_{true};
    Matrix3x4 localToWorld_{Matrix3x4::IDENTITY};
    Matrix3x4 worldToLocal_{Matrix3x4::IDENTITY};

    /// Character body parts.
    std::vector<BodyPart> bodyParts_;

    /// State machine via fuzzy pattern matching.
    PatternCollection stateMachine_;
};

}
