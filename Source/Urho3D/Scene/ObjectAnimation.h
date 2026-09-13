// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Resource/Resource.h"
#include "../Scene/AnimationDefs.h"

namespace Urho3D
{

class Archive;
class ArchiveBlock;
class ValueAnimation;
class ValueAnimationInfo;
class XMLElement;
class JSONValue;

/// Object animation class, an object animation include one or more attribute animations and theirs wrap mode and speed for an Animatable object.
class URHO3D_API ObjectAnimation : public Resource
{
    URHO3D_OBJECT(ObjectAnimation, Resource);

public:
    /// Construct.
    explicit ObjectAnimation(Context* context);
    /// Destruct.
    ~ObjectAnimation() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Save resource. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Load from XML data. Return true if successful.
    bool LoadXML(const XMLElement& source);
    /// Save as XML data. Return true if successful.
    bool SaveXML(XMLElement& dest) const;
    /// Load from JSON data. Return true if successful.
    bool LoadJSON(const JSONValue& source);
    /// Save as JSON data. Return true if successful.
    bool SaveJSON(JSONValue& dest) const;

    /// Add attribute animation, attribute name can in following format: "attribute" or "#0/#1/attribute" or ""#0/#1/@component#1/attribute.
    void AddAttributeAnimation
        (const ea::string& name, ValueAnimation* attributeAnimation, WrapMode wrapMode = WM_LOOP, float speed = 1.0f);
    /// Remove attribute animation, attribute name can in following format: "attribute" or "#0/#1/attribute" or ""#0/#1/@component#1/attribute.
    void RemoveAttributeAnimation(const ea::string& name);
    /// Remove attribute animation.
    void RemoveAttributeAnimation(ValueAnimation* attributeAnimation);

    /// Return attribute animation by name.
    /// @property{get_attributeAnimations}
    ValueAnimation* GetAttributeAnimation(const ea::string& name) const;
    /// Return attribute animation wrap mode by name.
    /// @property{get_wrapModes}
    WrapMode GetAttributeAnimationWrapMode(const ea::string& name) const;
    /// Return attribute animation speed by name.
    /// @property{get_speeds}
    float GetAttributeAnimationSpeed(const ea::string& name) const;

    /// Return all attribute animations infos.
    const ea::unordered_map<ea::string, SharedPtr<ValueAnimationInfo> >& GetAttributeAnimationInfos() const { return attributeAnimationInfos_; }

    /// Return attribute animation info by name.
    ValueAnimationInfo* GetAttributeAnimationInfo(const ea::string& name) const;

private:
    /// Send attribute animation added event.
    void SendAttributeAnimationAddedEvent(const ea::string& name);
    /// Send attribute animation remove event.
    void SendAttributeAnimationRemovedEvent(const ea::string& name);

    /// Name to attribute animation info mapping.
    ea::unordered_map<ea::string, SharedPtr<ValueAnimationInfo> > attributeAnimationInfos_;
};

}
