// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "CommonUtils.h"

#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/Input/InputEvents.h"
#include "Urho3D/Input/Input.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/IO/IOEvents.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Serializable.h>

#include <iostream>

namespace Tests
{
namespace
{

void PrintError(VariantMap& args)
{
    if (args[LogMessage::P_LEVEL].GetInt() == LOG_ERROR)
    {
        auto message = args[LogMessage::P_MESSAGE].GetString();
        std::cerr << "ERROR: " << message.c_str() << std::endl;
    }
}

}

static SharedPtr<Context> sharedContext;
static CreateContextCallback sharedContextCallback;

SharedPtr<Context> GetOrCreateContext(CreateContextCallback callback)
{
    if (sharedContext && sharedContextCallback == callback)
        return sharedContext;

    ResetContext();

    sharedContext = callback();
    sharedContextCallback = callback;
    return sharedContext;
}

void ResetContext()
{
    sharedContext = nullptr;
    sharedContextCallback = nullptr;
}

SharedPtr<Context> CreateCompleteContext()
{
    auto context = MakeShared<Context>();
    auto engine = new Engine(context);
    auto fs = context->GetSubsystem<FileSystem>();
    auto exeDir = GetParentPath(fs->GetProgramFileName());
    StringVariantMap parameters;
    parameters[EP_HEADLESS] = true;
    parameters[EP_LOG_QUIET] = true;
    parameters[EP_RESOURCE_PATHS] = "CoreData;Data";
    parameters[EP_RESOURCE_PREFIX_PATHS] = Format("{};{}", exeDir, GetParentPath(exeDir));
    parameters[EP_LOG_NAME] = "";
    const bool engineInitialized = engine->Initialize(parameters, {});

    engine->SubscribeToEvent(E_LOGMESSAGE, PrintError);
    REQUIRE(engineInitialized);
    return context;
}

void RunFrame(Context* context, float timeStep, float maxTimeStep)
{
    auto engine = context->GetSubsystem<Engine>();
    do
    {
        const float subTimeStep = ea::min(timeStep, maxTimeStep);
        engine->SetNextTimeStep(subTimeStep);
        engine->RunFrame();
        timeStep -= subTimeStep;
    }
    while (timeStep > 0.0f);
}

Resource* GetOrCreateResource(
    Context* context, StringHash type, const ea::string& name, ea::function<SharedPtr<Resource>(Context*)> factory)
{
    auto cache = context->GetSubsystem<ResourceCache>();
    if (auto resource = cache->GetResource(type, name, false))
        return resource;

    auto resource = factory(context);
    resource->SetName(name);
    cache->AddManualResource(resource);
    return resource;
}

void SendMouseMoveEvent(Input* input, IntVector2 pos, IntVector2 delta)
{
    using namespace MouseMove;

    VariantMap args;
    args[P_BUTTONS] = 0;
    args[P_QUALIFIERS] = 0;
    args[P_X] = pos.x_;
    args[P_Y] = pos.y_;
    args[P_DX] = delta.x_;
    args[P_DY] = delta.y_;
    input->SendEvent(E_MOUSEMOVE, args);
}

void SendKeyEvent(Input* input, StringHash eventId, Scancode scancode, Key key)
{
    using namespace KeyDown;

    VariantMap args;
    args[P_BUTTONS] = 0;
    args[P_QUALIFIERS] = 0;
    args[P_KEY] = key;
    args[P_SCANCODE] = scancode;
    args[P_REPEAT] = false;
    input->SendEvent(eventId, args);
}

void SendDPadEvent(Input* input, HatPosition position, int hatIndex, int joystickId)
{
    using namespace JoystickHatMove;

    VariantMap args;
    args[P_JOYSTICKID] = joystickId;
    args[P_HAT] = hatIndex;
    args[P_POSITION] = (int)position;
    input->SendEvent(E_JOYSTICKHATMOVE, args);
}

void SendJoystickDisconnected(Input* input, int joystickId)
{
    using namespace JoystickDisconnected;

    VariantMap args;
    args[P_JOYSTICKID] = joystickId;
    input->SendEvent(E_JOYSTICKDISCONNECTED, args);
}

void SendAxisEvent(Input* input, int axis, float value, int joystickId)
{
    using namespace JoystickAxisMove;

    VariantMap args;
    args[P_JOYSTICKID] = joystickId;
    args[P_AXIS] = axis;
    args[P_POSITION] = value;
    input->SendEvent(E_JOYSTICKAXISMOVE, args);
}

FrameEventTracker::FrameEventTracker(Context* context)
    : FrameEventTracker(context, E_ENDFRAMEPRIVATE)
{
}

FrameEventTracker::FrameEventTracker(Context* context, StringHash endFrameEventType)
    : Object(context)
{
    SubscribeToEvent(endFrameEventType,
        [&]
    {
        if (!recordEvents_)
            recordEvents_ = true;
        else
        {
            recordedFrames_.push_back(ea::move(currentFrameEvents_));
            currentFrameEvents_.clear();
        }
    });
}

void FrameEventTracker::TrackEvent(StringHash eventType)
{
    SubscribeToEvent(eventType, URHO3D_HANDLER(FrameEventTracker, HandleEvent));
}

void FrameEventTracker::TrackEvent(Object* object, StringHash eventType)
{
    SubscribeToEvent(object, eventType, URHO3D_HANDLER(FrameEventTracker, HandleEvent));
}

void FrameEventTracker::SkipFramesUntilEvent(StringHash eventType, unsigned hits)
{
    SkipFramesUntil(
        [&](const ea::vector<EventRecord>& events)
    {
        const auto isExpectedEvent = [&](const EventRecord& record) { return record.eventType_ == eventType; };
        if (ea::find_if(events.begin(), events.end(), isExpectedEvent) != events.end())
            --hits;
        return hits == 0;
    });
}

void FrameEventTracker::ValidatePattern(const ea::vector<ea::vector<StringHash>>& pattern) const
{
    const unsigned numFrames = recordedFrames_.size();
    REQUIRE(numFrames >= pattern.size());

    for (unsigned i = 0; i < numFrames; ++i)
    {
        const auto& frameEvents = recordedFrames_[i];
        const auto& framePattern = pattern[i % pattern.size()];
        REQUIRE(frameEvents.size() == framePattern.size());
        for (unsigned j = 0; j < framePattern.size(); ++j)
            REQUIRE(frameEvents[j].eventType_ == framePattern[j]);
    }
}

void FrameEventTracker::HandleEvent(StringHash eventType, VariantMap& eventData)
{
    currentFrameEvents_.push_back(EventRecord{eventType, eventData});
}

AttributeTracker::AttributeTracker(Context* context)
    : AttributeTracker(context, E_ENDFRAMEPRIVATE)
{
}

AttributeTracker::AttributeTracker(Context* context, StringHash endFrameEventType)
    : Object(context)
{
    SubscribeToEvent(endFrameEventType,
        [&]
    {
        for (const auto& [serializable, attributeName] : trackers_)
        {
            if (!serializable)
                recordedValues_.push_back(Variant::EMPTY);
            else
                recordedValues_.push_back(serializable->GetAttribute(attributeName));
        }
    });
}

void AttributeTracker::Track(Serializable* serializable, const ea::string& attributeName)
{
    trackers_.emplace_back(WeakPtr<Serializable>(serializable), attributeName);
}

}
