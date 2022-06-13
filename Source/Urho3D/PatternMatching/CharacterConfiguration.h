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

#include "../Graphics/StaticModel.h"
#include "../Resource/Resource.h"
#include "PatternCollection.h"
#include "PatternIndex.h"

namespace Urho3D
{
class Model;
class Material;
class PatternCollection;

struct URHO3D_API CharacterBodyPart
{
    /// Serialize from/to archive. Return true if successful.
    void SerializeInBlock(Archive& archive);

    /// Name of the body part.
    ea::string name_;
    /// Is model static or animated.
    bool static_{};

    /// Bone name to attach to.
    ea::string attachmentBone_;

    /// Model selector via fuzzy pattern matching.
    PatternCollection variants_;

    /// Indexed model selector
    PatternIndex variantIndex_;
};

struct URHO3D_API CharacterBodyPartInstance
{
    /// Primary animated or static model instance.
    SharedPtr<StaticModel> primaryModel_;
    /// Secondary (outline, shadow, etc) animated or static model instance.
    SharedPtr<StaticModel> secondaryModel_;
    /// Last matching variation match index.
    int lastQueryResult{-1};
    /// Body part attached to root.
    bool attachedToRoot_{true};

    void SetModel(ResourceRef model, const ResourceRefList& materials);
    void SetSecondaryMaterial(Material* material);
    void Reset();
};

/// Character configuration resource.
class URHO3D_API CharacterConfiguration : public Resource
{
    URHO3D_OBJECT(CharacterConfiguration, Resource)

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

    /// Update pattern indices to query.
    void Commit();

    /// Resize body parts vector.
    void SetNumBodyParts(unsigned num);
    /// Get number of body parts.
    unsigned GetNumBodyParts() const;
    /// Get total number of body parts including parent body parts.
    unsigned GetTotalNumBodyParts() const;

    /// Return all body parts.
    const ea::vector<CharacterBodyPart>& GetBodyParts() const { return bodyParts_; }

    /// Return modifiable body parts.
    ea::vector<CharacterBodyPart>& GetModifiableBodyParts() { return bodyParts_; }

    /// Add new metadata variable or overwrite old value.
    void AddMetadata(const ea::string& name, const Variant& value);
    /// Remove metadata variable.
    void RemoveMetadata(const ea::string& name);
    /// Remove all metadata variables.
    void RemoveAllMetadata();
    /// Return metadata variable.
    const Variant& GetMetadata(const ea::string& name) const;

    void SetModel(Model* model);
    void SetModelAttr(ResourceRef model);
    ResourceRef GetModelAttr() const { return model_; }
    /// Get model from configuration or from parent configuration.
    ResourceRef GetActualModelAttr() const;

    void SetMaterial(Material* material);
    void SetMaterialAttr(const ResourceRefList& materials);
    const ResourceRefList& GetMaterialAttr() const { return material_; }
    /// Get material list from configuration or from parent configuration.
    const ResourceRefList& GetActualMaterialAttr() const;

    /// Set parent configuration reference. All states and body parts for it will be appended to the current configuration.
    void SetParentAttr(ResourceRef parent);
    /// Get parent configuration reference.
    ResourceRef GetParentAttr() const { return parentConfiguration_; }
    /// Set parent configuration. All states and body parts for it will be appended to the current
    /// configuration.
    void SetParent(CharacterConfiguration* parent);
    /// Get parent configuration.
    CharacterConfiguration* GetParent() const;

    /// Get pattern state machine configuration.
    PatternCollection* GetStates();

    /// Get pattern state machine index to query.
    PatternIndex* GetIndex();

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

    const Matrix3x4& LocalToWorld() { return localToWorld_; }
    const Matrix3x4& WorldToLocal() { return worldToLocal_; }

    CharacterBodyPartInstance CreateBodyPartModelComponent(const CharacterBodyPart& bodyPart, Node* root) const;
    void UpdateBodyPart(CharacterBodyPartInstance& instance, const CharacterBodyPart& bodyPart,
        const PatternQuery& query, Material* secondaryMaterial) const;
    void SetBodyPartModel(CharacterBodyPartInstance& instance, const VariantMap eventArgs) const;

private:
    /// Reset to defaults.
    void ResetToDefaults();
    /// Update matrices.
    void UpdateMatrices();

    /// Master model that has complete bone structure 
    ResourceRef model_;
    /// Master model materials
    ResourceRefList material_;

    /// Parent CharacterConfiguration
    ResourceRef parentConfiguration_;
    /// Model offset
    Vector3 position_{Vector3::ZERO};
    /// Model rotation
    Quaternion rotation_{Quaternion::IDENTITY};
    /// Model scale
    Vector3 scale_{Vector3::ONE};
    /// Model cast shadow property
    bool castShadows_{true};
    Matrix3x4 localToWorld_{Matrix3x4::IDENTITY};
    Matrix3x4 worldToLocal_{Matrix3x4::IDENTITY};

    /// Character body parts.
    ea::vector<CharacterBodyPart> bodyParts_;

    /// State machine implemented via fuzzy pattern matching.
    PatternCollection states_;
    /// Pattern index to query.
    PatternIndex stateIndex_;
    /// Cached parent state pointer.
    SharedPtr<CharacterConfiguration> parent_;
    /// Configuration metadata.
    StringVariantMap metadata_;
};

}
