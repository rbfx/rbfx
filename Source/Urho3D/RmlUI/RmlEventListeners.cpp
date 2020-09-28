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
#include "../Precompiled.h"

#include "../Audio/Sound.h"
#include "../Audio/SoundSource.h"
#include "../Core/Context.h"
#include "../Resource/JSONFile.h"
#include "../IO/Log.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Node.h"
#include "../RmlUI/RmlUI.h"
#include "../RmlUI/RmlEventListeners.h"

#include "../DebugNew.h"

namespace Urho3D
{

namespace Detail
{

SoundEventListener* SoundEventListener::CreateInstancer(const ea::string& value, Rml::Element* element)
{
    static ea::string prefix = "sound:";
    if (!value.starts_with(prefix))
        return nullptr;

    ea::string soundResource = value.substr(prefix.length());
    return new SoundEventListener(soundResource);
}

SoundEventListener::SoundEventListener(const ea::string& soundResource)
    : soundResource_(soundResource)
{
}

void SoundEventListener::ProcessEvent(Rml::Event& event)
{
    if (soundNode_.Null())
    {
        // At the time event listener is instantiated, element is not added to DOM, so it does not have rml context set and we have no means
        // to find Urho3D::Context. That is why initialization of these objects is delayed until first event invocation.
        Detail::RmlContext* rmlContext = static_cast<Detail::RmlContext*>(event.GetTargetElement()->GetContext());
        RmlUI* ui = rmlContext->GetOwnerSubsystem();
        Context* context = ui->GetContext();
        soundNode_ = context->CreateObject<Node>();
        soundPlayer_ = soundNode_->CreateComponent<SoundSource>();
    }

    ResourceCache* cache = soundNode_->GetSubsystem<ResourceCache>();
    if (Sound* sound = cache->GetResource<Sound>(soundResource_))
        soundPlayer_->Play(sound);
}

void SoundEventListener::OnDetach(Rml::Element* element)
{
    (void)element;
    delete this;
}

static Variant ToVariant(const JSONValue& json)
{
    switch (json.GetValueType())
    {
    case JSON_NULL: return Variant();
    case JSON_BOOL: return json.GetBool();
    case JSON_NUMBER:
    {
        switch (json.GetNumberType())
        {
        case JSONNT_NAN: return Variant(NAN);
        case JSONNT_INT: return json.GetInt();
        case JSONNT_UINT: return json.GetUInt();
        case JSONNT_FLOAT_DOUBLE: return json.GetFloat();
        default: assert(0); break;
        }
        break;
    }
    case JSON_STRING: return json.GetString();
    case JSON_ARRAY:
    {
        VariantVector array;
        for (int i = 0; i < json.Size(); i++)
            array.push_back(ToVariant(json[i]));
        return array;
    }
    case JSON_OBJECT:
    {
        VariantMap map;
        for (auto it : json)
            map[it.first] = ToVariant(it.second);
        return map;
    }
    default:
        assert(0);
        break;
    }
    return Variant();
}

CustomEventListener* CustomEventListener::CreateInstancer(const ea::string& value, Rml::Element* element)
{
    if (!value.starts_with("{") && !value.starts_with("//{"))
        return nullptr;

    ea::string quotesFixedValue = value.starts_with("//") ? value.substr(2) : value;
    quotesFixedValue.replace("'", "\"");
    quotesFixedValue.replace("&quot;", "\\\"");
    JSONValue root;
    if (!JSONFile::ParseJSON(quotesFixedValue, root))
    {
        URHO3D_LOGERROR("Deserializing JSON event '{}' failed.", quotesFixedValue);
        return nullptr;
    }

    if (!root.IsObject())
    {
        URHO3D_LOGERROR("Custom event '{}' must be a valid JSON object.", quotesFixedValue);
        return nullptr;
    }

    if (!root.Contains("e"))
    {
        URHO3D_LOGERROR("Custom event '{}' must contain a key 'e' with event type.", quotesFixedValue);
        return nullptr;
    }

    return new CustomEventListener(root);
}

CustomEventListener::CustomEventListener(const JSONValue& value)
{
    for (auto it : value)
    {
        if (it.first == "e")
            event_ = it.second.GetString();
        else
            eventArgs_[it.first] = ToVariant(it.second);
    }
}

void CustomEventListener::ProcessEvent(Rml::Event& event)
{
    Detail::RmlContext* context = static_cast<Detail::RmlContext*>(event.GetCurrentElement()->GetContext());
    RmlUI* ui = context->GetOwnerSubsystem();
    VariantMap& args = ui->GetEventDataMap();
    args.insert(eventArgs_.begin(), eventArgs_.end());
    args["_Element"] = event.GetCurrentElement();
    args["_Phase"] = (int)event.GetPhase();
    args["_IsPropagating"] = event.IsPropagating();
    ui->SendEvent(event_, args);
}

void CustomEventListener::OnDetach(Rml::Element* element)
{
    (void)element;
    delete this;
}

}   // namespace Detail

}   // namespace Urho3D
