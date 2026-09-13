// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Archive.h"
#include "Urho3D/IO/ArchiveSerialization.h"
#include "Urho3D/Resource/XMLFile.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/Scene/ObjectAnimation.h"
#include "Urho3D/Scene/SceneEvents.h"
#include "Urho3D/Scene/ValueAnimation.h"
#include "Urho3D/Scene/ValueAnimationInfo.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

const char* wrapModeNames[] =
{
    "Loop",
    "Once",
    "Clamp",
    nullptr
};

ObjectAnimation::ObjectAnimation(Context* context) :
    Resource(context)
{
}

ObjectAnimation::~ObjectAnimation() = default;

void ObjectAnimation::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ObjectAnimation>();
}

bool ObjectAnimation::BeginLoad(Deserializer& source)
{
    XMLFile xmlFile(context_);
    if (!xmlFile.Load(source))
        return false;

    return LoadXML(xmlFile.GetRoot());
}

bool ObjectAnimation::Save(Serializer& dest) const
{
    XMLFile xmlFile(context_);

    XMLElement rootElem = xmlFile.CreateRoot("objectanimation");
    if (!SaveXML(rootElem))
        return false;

    return xmlFile.Save(dest);
}

bool ObjectAnimation::LoadXML(const XMLElement& source)
{
    attributeAnimationInfos_.clear();

    XMLElement animElem;
    animElem = source.GetChild("attributeanimation");
    while (animElem)
    {
        ea::string name = animElem.GetAttribute("name");

        SharedPtr<ValueAnimation> animation(MakeShared<ValueAnimation>(context_));
        if (!animation->LoadXML(animElem))
            return false;

        ea::string wrapModeString = animElem.GetAttribute("wrapmode");
        WrapMode wrapMode = WM_LOOP;
        for (int i = 0; i <= WM_CLAMP; ++i)
        {
            if (wrapModeString == wrapModeNames[i])
            {
                wrapMode = (WrapMode)i;
                break;
            }
        }

        float speed = animElem.GetFloat("speed");
        AddAttributeAnimation(name, animation, wrapMode, speed);

        animElem = animElem.GetNext("attributeanimation");
    }

    return true;
}

bool ObjectAnimation::SaveXML(XMLElement& dest) const
{
    for (auto i = attributeAnimationInfos_.begin();
         i != attributeAnimationInfos_.end(); ++i)
    {
        XMLElement animElem = dest.CreateChild("attributeanimation");
        animElem.SetAttribute("name", i->first);

        const ValueAnimationInfo* info = i->second;
        if (!info->GetAnimation()->SaveXML(animElem))
            return false;

        animElem.SetAttribute("wrapmode", wrapModeNames[info->GetWrapMode()]);
        animElem.SetFloat("speed", info->GetSpeed());
    }

    return true;
}

bool ObjectAnimation::LoadJSON(const JSONValue& source)
{
    attributeAnimationInfos_.clear();

    JSONValue attributeAnimationsValue = source.Get("attributeanimations");
    if (attributeAnimationsValue.IsNull())
        return true;
    if (!attributeAnimationsValue.IsObject())
        return true;

    const JSONObject& attributeAnimationsObject = attributeAnimationsValue.GetObject();

    for (auto it = attributeAnimationsObject.begin(); it != attributeAnimationsObject.end(); it++)
    {
        ea::string name = it->first;
        JSONValue value = it->second;
        SharedPtr<ValueAnimation> animation(MakeShared<ValueAnimation>(context_));
        if (!animation->LoadJSON(value))
            return false;

        ea::string wrapModeString = value.Get("wrapmode").GetString();
        WrapMode wrapMode = WM_LOOP;
        for (int i = 0; i <= WM_CLAMP; ++i)
        {
            if (wrapModeString == wrapModeNames[i])
            {
                wrapMode = (WrapMode)i;
                break;
            }
        }

        float speed = value.Get("speed").GetFloat();
        AddAttributeAnimation(name, animation, wrapMode, speed);
    }

    return true;
}

bool ObjectAnimation::SaveJSON(JSONValue& dest) const
{
    JSONValue attributeAnimationsValue;

    for (auto i = attributeAnimationInfos_.begin();
         i != attributeAnimationInfos_.end(); ++i)
    {
        JSONValue animValue;
        animValue.Set("name", i->first);

        const ValueAnimationInfo* info = i->second;
        if (!info->GetAnimation()->SaveJSON(animValue))
            return false;

        animValue.Set("wrapmode", wrapModeNames[info->GetWrapMode()]);
        animValue.Set("speed", (float) info->GetSpeed());

        attributeAnimationsValue.Set(i->first, animValue);
    }

    dest.Set("attributeanimations", attributeAnimationsValue);
    return true;
}

void ObjectAnimation::AddAttributeAnimation(const ea::string& name, ValueAnimation* attributeAnimation, WrapMode wrapMode, float speed)
{
    if (!attributeAnimation)
        return;

    attributeAnimation->SetOwner(this);
    attributeAnimationInfos_[name] = new ValueAnimationInfo(attributeAnimation, wrapMode, speed);

    SendAttributeAnimationAddedEvent(name);
}

void ObjectAnimation::RemoveAttributeAnimation(const ea::string& name)
{
    auto i = attributeAnimationInfos_.find(
        name);
    if (i != attributeAnimationInfos_.end())
    {
        SendAttributeAnimationRemovedEvent(name);

        i->second->GetAnimation()->SetOwner(nullptr);
        attributeAnimationInfos_.erase(i);
    }
}

void ObjectAnimation::RemoveAttributeAnimation(ValueAnimation* attributeAnimation)
{
    if (!attributeAnimation)
        return;

    for (auto i = attributeAnimationInfos_.begin();
         i != attributeAnimationInfos_.end(); ++i)
    {
        if (i->second->GetAnimation() == attributeAnimation)
        {
            SendAttributeAnimationRemovedEvent(i->first);

            attributeAnimation->SetOwner(nullptr);
            attributeAnimationInfos_.erase(i);
            return;
        }
    }
}

ValueAnimation* ObjectAnimation::GetAttributeAnimation(const ea::string& name) const
{
    ValueAnimationInfo* info = GetAttributeAnimationInfo(name);
    return info ? info->GetAnimation() : nullptr;
}

WrapMode ObjectAnimation::GetAttributeAnimationWrapMode(const ea::string& name) const
{
    ValueAnimationInfo* info = GetAttributeAnimationInfo(name);
    return info ? info->GetWrapMode() : WM_LOOP;
}

float ObjectAnimation::GetAttributeAnimationSpeed(const ea::string& name) const
{
    ValueAnimationInfo* info = GetAttributeAnimationInfo(name);
    return info ? info->GetSpeed() : 1.0f;
}

ValueAnimationInfo* ObjectAnimation::GetAttributeAnimationInfo(const ea::string& name) const
{
    auto i = attributeAnimationInfos_.find(
        name);
    if (i != attributeAnimationInfos_.end())
        return i->second;
    return nullptr;
}

void ObjectAnimation::SendAttributeAnimationAddedEvent(const ea::string& name)
{
    using namespace AttributeAnimationAdded;
    VariantMap& eventData = GetEventDataMap();
    eventData[P_OBJECTANIMATION] = this;
    eventData[P_ATTRIBUTEANIMATIONNAME] = name;
    SendEvent(E_ATTRIBUTEANIMATIONADDED, eventData);
}

void ObjectAnimation::SendAttributeAnimationRemovedEvent(const ea::string& name)
{
    using namespace AttributeAnimationRemoved;
    VariantMap& eventData = GetEventDataMap();
    eventData[P_OBJECTANIMATION] = this;
    eventData[P_ATTRIBUTEANIMATIONNAME] = name;
    SendEvent(E_ATTRIBUTEANIMATIONREMOVED, eventData);
}

}
