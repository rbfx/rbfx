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
#include "CharacterConfiguration.h"
#include "../Scene/Component.h"
#include "../Graphics/AnimatedModel.h"
#include "../Graphics/AnimationController.h"

namespace Urho3D
{

/// Character configurator component.
class URHO3D_API CharacterConfigurator: public Component
{
    URHO3D_OBJECT(CharacterConfigurator, Component)
private:
    struct BodyPart
    {
        int lastMatch_{-1};
        const CharacterConfiguration* configuration_{};
        unsigned index_{};
        CharacterBodyPartInstance modelComponent_;
    };

public:
    /// Construct.
    CharacterConfigurator(Context* context);
    /// Destruct.
    ~CharacterConfigurator() override;
    /// Register object attributes. Drawable must be registered first.
    /// @nobind
    static void RegisterObject(Context* context);

    void Update(const PatternQuery& query);

    /// Handle enabled/disabled state change.
    //void OnSetEnabled() override;

    /// Set configuration.
    void SetConfiguration(CharacterConfiguration* configuration);
    /// Return model.
    CharacterConfiguration* GetConfiguration() const { return configuration_; }

    /// Set configuration attribute.
    void SetConfigurationAttr(const ResourceRef& value);
    /// Return configuration attribute.
    ResourceRef GetConfigurationAttr() const;

    /// Set secondary material.
    void SetSecondaryMaterial(Material* material);
    /// Return secondary material.
    Material* GetSecondaryMaterial() const { return secondaryMaterial_; }

    /// Set secondary material attribute.
    void SetSecondaryMaterialAttr(const ResourceRef& value);
    /// Return secondary material attribute.
    ResourceRef GetSecondaryMaterialAttr() const;

    /// Get linear velocity from current animation metadata.
    const Vector3& GetLinearVelocity() const { return velocity_; }

    /// Handle enabled/disabled state change.
    virtual void OnSetEnabled() override;
    /// Handle scene node being assigned at creation.
    virtual void OnNodeSet(Node* node) override;

private:
    /// Handle configuration reload finished.
    void HandleConfigurationReloadFinished(StringHash eventType, VariantMap& eventData);

    void ResizeBodyParts(unsigned num);
    void ResetBodyStructure();
    void ResetMasterModel();
    void ResetBodyPartModels(ea::span<BodyPart> parts, const CharacterConfiguration* configuration, const PatternQuery& query);

    void PlayAnimation(StringHash eventType, const VariantMap& eventData);

    /// Configuration.
    SharedPtr<CharacterConfiguration> configuration_;
    /// Shadow material.
    SharedPtr<Material> secondaryMaterial_;

    SharedPtr<Node> characterNode_;
    ea::vector<BodyPart> bodyPartNodes_;
    CharacterBodyPartInstance masterModel_;
    SharedPtr<AnimationController> animationController_;
    /// Velocity in master model local space.
    Vector3 velocity_ {Vector3::ZERO};

    /// Last query applied to restore state from scene xml file.
    VariantMap savedQuery_;

    int stateIndex_{-1};
};

}
