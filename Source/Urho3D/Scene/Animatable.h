//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include <EASTL/unique_ptr.h>
#include <EASTL/hash_set.h>

#include "../Scene/Serializable.h"
#include "../Scene/ValueAnimationInfo.h"

namespace Urho3D
{

class Animatable;
class ValueAnimation;
class AttributeAnimationInfo;
class ObjectAnimation;

/// Attribute animation instance.
class URHO3D_API AttributeAnimationInfo : public ValueAnimationInfo
{
public:
    /// Construct.
    AttributeAnimationInfo
        (Animatable* animatable, const AttributeInfo& attributeInfo, ValueAnimation* attributeAnimation, WrapMode wrapMode,
            float speed);
    /// Copy construct.
    AttributeAnimationInfo(const AttributeAnimationInfo& other);
    /// Destruct.
    ~AttributeAnimationInfo() override;

    /// Return attribute information.
    const AttributeInfo& GetAttributeInfo() const { return attributeInfo_; }

protected:
    /// Apply new animation value to the target object. Called by Update().
    void ApplyValue(const Variant& newValue) override;

private:
    /// Attribute information.
    const AttributeInfo& attributeInfo_;
};

/// Base class for animatable object, an animatable object can be set animation on it's attributes, or can be set an object animation to it.
class URHO3D_API Animatable : public Serializable
{
    URHO3D_OBJECT(Animatable, Serializable);

public:
    /// Construct.
    explicit Animatable(Context* context);
    /// Destruct.
    ~Animatable() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load from XML data. Return true if successful.
    bool LoadXML(const XMLElement& source) override;
    /// Save as XML data. Return true if successful.
    bool SaveXML(XMLElement& dest) const override;
    /// Load from JSON data. Return true if successful.
    bool LoadJSON(const JSONValue& source) override;
    /// Save as JSON data. Return true if successful.
    bool SaveJSON(JSONValue& dest) const override;

    /// Set automatic update of animation, default true.
    /// @property
    void SetAnimationEnabled(bool enable);
    /// Set time position of all attribute animations or an object animation manually. Automatic update should be disabled in this case.
    void SetAnimationTime(float time);

    /// Set object animation.
    /// @property
    void SetObjectAnimation(ObjectAnimation* objectAnimation);
    /// Set attribute animation.
    void SetAttributeAnimation
        (const ea::string& name, ValueAnimation* attributeAnimation, WrapMode wrapMode = WM_LOOP, float speed = 1.0f);
    /// Set attribute animation wrap mode.
    void SetAttributeAnimationWrapMode(const ea::string& name, WrapMode wrapMode);
    /// Set attribute animation speed.
    void SetAttributeAnimationSpeed(const ea::string& name, float speed);
    /// Set attribute animation time position manually. Automatic update should be disabled in this case.
    void SetAttributeAnimationTime(const ea::string& name, float time);
    /// Remove object animation. Same as calling SetObjectAnimation with a null pointer.
    void RemoveObjectAnimation();
    /// Remove attribute animation. Same as calling SetAttributeAnimation with a null pointer.
    void RemoveAttributeAnimation(const ea::string& name);

    /// Return animation enabled.
    /// @property
    bool GetAnimationEnabled() const { return animationEnabled_; }

    /// Return object animation.
    /// @property
    ObjectAnimation* GetObjectAnimation() const;
    /// Return attribute animation.
    ValueAnimation* GetAttributeAnimation(const ea::string& name) const;
    /// Return attribute animation wrap mode.
    WrapMode GetAttributeAnimationWrapMode(const ea::string& name) const;
    /// Return attribute animation speed.
    float GetAttributeAnimationSpeed(const ea::string& name) const;
    /// Return attribute animation time position.
    float GetAttributeAnimationTime(const ea::string& name) const;

    /// Set object animation attribute.
    void SetObjectAnimationAttr(const ResourceRef& value);
    /// Return object animation attribute.
    ResourceRef GetObjectAnimationAttr() const;

protected:
    /// Handle attribute animation added.
    virtual void OnAttributeAnimationAdded() = 0;
    /// Handle attribute animation removed.
    virtual void OnAttributeAnimationRemoved() = 0;
    /// Find target of an attribute animation from object hierarchy by name.
    virtual Animatable* FindAttributeAnimationTarget(const ea::string& name, ea::string& outName);
    /// Set object attribute animation internal.
    void SetObjectAttributeAnimation(const ea::string& name, ValueAnimation* attributeAnimation, WrapMode wrapMode, float speed);
    /// Handle object animation added.
    void OnObjectAnimationAdded(ObjectAnimation* objectAnimation);
    /// Handle object animation removed.
    void OnObjectAnimationRemoved(ObjectAnimation* objectAnimation);
    /// Update attribute animations.
    void UpdateAttributeAnimations(float timeStep);
    /// Return attribute animation info.
    AttributeAnimationInfo* GetAttributeAnimationInfo(const ea::string& name) const;
    /// Handle attribute animation added.
    void HandleAttributeAnimationAdded(StringHash eventType, VariantMap& eventData);
    /// Handle attribute animation removed.
    void HandleAttributeAnimationRemoved(StringHash eventType, VariantMap& eventData);

    /// Animation enabled.
    bool animationEnabled_;
    /// Animation.
    SharedPtr<ObjectAnimation> objectAnimation_;
    /// Attribute animation infos.
    ea::unordered_map<ea::string, SharedPtr<AttributeAnimationInfo> > attributeAnimationInfos_;
};

}
