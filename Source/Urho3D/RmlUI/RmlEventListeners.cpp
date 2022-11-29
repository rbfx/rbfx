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

#include "../RmlUI/RmlEventListeners.h"

#include "../Audio/Sound.h"
#include "../Audio/SoundSource.h"
#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Resource/JSONFile.h"
#include "../Resource/ResourceCache.h"
#include "../RmlUI/RmlNavigationManager.h"
#include "../RmlUI/RmlUI.h"
#include "../RmlUI/RmlUIComponent.h"
#include "../Scene/Node.h"

#include <regex>

#include "../DebugNew.h"

namespace Urho3D
{

namespace Detail
{

namespace
{

StringVector ParsePipe(ea::string_view str)
{
    StringVector result;
    result.push_back(EMPTY_STRING);

    bool isEscaped = false;
    for (char ch : str)
    {
        if (ch == '\'')
            isEscaped = !isEscaped;
        if (!isEscaped && ch == ';')
            result.push_back(EMPTY_STRING);
        else
            result.back() += ch;
    }

    for (auto& str : result)
        str.trim();
    ea::erase_if(result, [](const ea::string& str) { return str.empty(); });
    return result;
}

ea::optional<ea::pair<ea::string, float>> ParseSound(ea::string_view str)
{
    static const std::regex r(R"(\s*(\d+%)?\s*(.+)\s*)");

    std::cmatch match;
    if (!std::regex_match(str.begin(), str.end(), match, r))
        return ea::nullopt;

    const ea::string resource = match.str(2).c_str();
    const float volume = match.str(1).empty() ? 1.0f : FromString<int>(match.str(1).c_str()) / 100.0f;
    return ea::make_pair(resource, volume);
}

ea::optional<ea::string> ParseNavigate(ea::string_view str)
{
    static const std::regex r(R"(\s*(push|pop)\s*(?:\(\s*(\w+)\s*\))?\s*)");

    std::cmatch match;
    if (!std::regex_match(str.begin(), str.end(), match, r))
        return ea::nullopt;

    if (match.str(1) == "push")
    {
        const std::string group = match.str(2);
        return ea::string(group.data(), group.size());
    }
    else if (match.str(1) == "pop")
        return EMPTY_STRING;
    else
        return ea::nullopt;
}

Variant ToVariant(const JSONValue& json)
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

ea::pair<ea::string, VariantMap> ParseEvent(ea::string_view str)
{
    const ea::string_view::size_type pos = str.find('(');
    if (pos == ea::string_view::npos)
        return ea::make_pair(ea::string(str).trimmed(), VariantMap{});

    // TODO(ui): support event parameters (in JSON?)
    URHO3D_LOGWARNING("TODO: Event parameters are not supported yet");

    const auto eventName = ea::string(str.substr(0, pos)).trimmed();
    return ea::make_pair(eventName, VariantMap{});
}

}

Rml::EventListener* CreateSingleEventListener(ea::string_view value, Rml::Element* element)
{
    static const ea::string_view navigatePrefix = "navigate:";
    static const ea::string_view soundPrefix = "sound:";
    static const ea::string_view eventPrefix = "event:";

    if (value.starts_with(navigatePrefix))
        return NavigateEventListener::CreateInstancer(value.substr(navigatePrefix.size()), element);
    else if (value.starts_with(soundPrefix))
        return SoundEventListener::CreateInstancer(value.substr(soundPrefix.size()), element);
    else if (value.starts_with(eventPrefix))
        return CustomEventListener::CreateInstancer(value.substr(eventPrefix.size()), element);
    else
    {
        URHO3D_LOGWARNING("Unknown event '{}'", value);
        return nullptr;
    }
}

Rml::EventListener* PipeEventListener::CreateInstancer(ea::string_view value, Rml::Element* element)
{
    const StringVector parsed = ParsePipe(value);

    EventListenerVector listeners;
    for (const auto& str : parsed)
    {
        ea::unique_ptr<Rml::EventListener> listener{CreateSingleEventListener(str, element)};
        if (listener)
            listeners.push_back(ea::move(listener));
    }

    if (listeners.empty())
        return nullptr;
    else if (listeners.size() == 1)
        return listeners[0].release();
    else
        return new PipeEventListener(ea::move(listeners));
}

PipeEventListener::PipeEventListener(EventListenerVector&& listeners)
    : listeners_(ea::move(listeners))
{
}

void PipeEventListener::ProcessEvent(Rml::Event& event)
{
    for (auto& listener : listeners_)
        listener->ProcessEvent(event);
}

void PipeEventListener::OnDetach(Rml::Element* element)
{
    delete this;
}

Rml::EventListener* NavigateEventListener::CreateInstancer(ea::string_view value, Rml::Element* element)
{
    const auto group = ParseNavigate(value);
    if (!group)
    {
        URHO3D_LOGWARNING("Invalid syntax for navigate event: '{}'", value);
        return nullptr;
    }
    return new NavigateEventListener(*group);
}

NavigateEventListener::NavigateEventListener(const ea::string& group)
    : group_(group)
{
}

void NavigateEventListener::ProcessEvent(Rml::Event& event)
{
    if (Rml::Element* element = event.GetCurrentElement())
    {
        if (Rml::ElementDocument* document = element->GetOwnerDocument())
        {
            if (auto component = RmlUIComponent::FromDocument(document))
            {
                RmlNavigationManager& manager = component->GetNavigationManager();
                if (!group_.empty())
                    manager.PushCursorGroup(group_);
                else
                    manager.PopCursorGroup();
            }
        }
    }
}

void NavigateEventListener::OnDetach(Rml::Element* element)
{
    delete this;
}

Rml::EventListener* SoundEventListener::CreateInstancer(ea::string_view value, Rml::Element* element)
{
    const auto parsed = ParseSound(value);
    if (!parsed)
    {
        URHO3D_LOGWARNING("Invalid syntax of sound event: '{}'", value);
        return nullptr;
    }

    const auto& [soundResource, volume] = *parsed;
    return new SoundEventListener(soundResource, volume);
}

SoundEventListener::SoundEventListener(const ea::string& soundResource, float volume)
    : soundResource_(soundResource)
    , volume_(volume)
{
}

void SoundEventListener::ProcessEvent(Rml::Event& event)
{
    if (soundNode_ == nullptr)
    {
        // At the time event listener is instantiated, element is not added to DOM, so it does not have rml context set and we have no means
        // to find Urho3D::Context. That is why initialization of these objects is delayed until first event invocation.
        Detail::RmlContext* rmlContext = static_cast<Detail::RmlContext*>(event.GetTargetElement()->GetContext());
        RmlUI* ui = rmlContext->GetOwnerSubsystem();
        Context* context = ui->GetContext();
        soundNode_ = MakeShared<Node>(context);
        soundPlayer_ = soundNode_->CreateComponent<SoundSource>();
        soundPlayer_->SetGain(volume_);
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

Rml::EventListener* CustomEventListener::CreateInstancer(ea::string_view value, Rml::Element* element)
{
    const auto& [eventType, eventParams] = ParseEvent(value);
    return new CustomEventListener(eventType, eventParams);
}

CustomEventListener::CustomEventListener(const ea::string& eventType, const VariantMap& eventData)
    : eventType_(eventType)
    , eventData_(eventData)
{
}

void CustomEventListener::ProcessEvent(Rml::Event& event)
{
    Detail::RmlContext* context = static_cast<Detail::RmlContext*>(event.GetCurrentElement()->GetContext());
    RmlUI* ui = context->GetOwnerSubsystem();
    VariantMap& args = ui->GetEventDataMap();
    args.insert(eventData_.begin(), eventData_.end());
    args["_Element"] = event.GetCurrentElement();
    args["_Phase"] = (int)event.GetPhase();
    args["_IsPropagating"] = event.IsPropagating();
    ui->SendEvent(eventType_, args);
}

void CustomEventListener::OnDetach(Rml::Element* element)
{
    delete this;
}

}

}
