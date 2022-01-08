//
// Copyright (c) 2017-2021 the rbfx project.
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

#include "CommonUtils.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Serializable.h>

namespace Tests
{

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
    VariantMap parameters;
    parameters[EP_HEADLESS] = true;
    parameters[EP_LOG_QUIET] = true;
    const bool engineInitialized = engine->Initialize(parameters);
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

Resource* GetOrCreateResource(Context* context, StringHash type, const ea::string& name, ea::function<SharedPtr<Resource>()> factory)
{
    auto cache = context->GetSubsystem<ResourceCache>();
    if (auto resource = cache->GetResource(type, name))
        return resource;

    auto resource = factory();
    resource->SetName(name);
    cache->AddManualResource(resource);
    return resource;
}

FrameEventTracker::FrameEventTracker(Context* context)
    : FrameEventTracker(context, E_ENDFRAMEPRIVATE)
{
}

FrameEventTracker::FrameEventTracker(Context* context, StringHash endFrameEventType)
    : Object(context)
{
    SubscribeToEvent(endFrameEventType,
        [&](StringHash, VariantMap&)
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

void FrameEventTracker::ValidatePattern(ea::vector<ea::vector<StringHash>> pattern) const
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
        [&](StringHash, VariantMap&)
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
