// SPDX-License-Identifier: MIT

#include "Sequencer.h"

#include <algorithm>
#include <cmath>

namespace Urho3D
{

bool Sequencer::AddTrack(const ea::string& name, SequencerTrackType type, ea::string* error)
{
    if (name.empty())
    {
        if (error)
            *error = "Sequencer track name cannot be empty.";
        return false;
    }
    if (GetTrack(name))
    {
        if (error)
            *error = "Sequencer track already exists.";
        return false;
    }
    tracks_.push_back({name, type, {}, false});
    return true;
}

bool Sequencer::RemoveTrack(const ea::string& name)
{
    const auto iter = std::find_if(tracks_.begin(), tracks_.end(),
        [&](const SequencerTrack& track) { return track.name == name; });
    if (iter == tracks_.end())
        return false;
    tracks_.erase(iter);
    return true;
}

SequencerTrack* Sequencer::GetTrack(const ea::string& name)
{
    for (SequencerTrack& track : tracks_)
    {
        if (track.name == name)
            return &track;
    }
    return nullptr;
}

const SequencerTrack* Sequencer::GetTrack(const ea::string& name) const
{
    for (const SequencerTrack& track : tracks_)
    {
        if (track.name == name)
            return &track;
    }
    return nullptr;
}

bool Sequencer::AddKeyframe(const ea::string& trackName, const SequencerKeyframe& keyframe, ea::string* error)
{
    SequencerTrack* track = GetTrack(trackName);
    if (!track)
    {
        if (error)
            *error = "Sequencer track does not exist.";
        return false;
    }
    if (keyframe.time < 0.0f || keyframe.time > duration_)
    {
        if (error)
            *error = "Sequencer keyframe time is outside the sequence duration.";
        return false;
    }
    for (const SequencerKeyframe& existing : track->keyframes)
    {
        if (existing.time == keyframe.time)
        {
            if (error)
                *error = "Sequencer track already has a keyframe at this time.";
            return false;
        }
    }
    track->keyframes.push_back(keyframe);
    std::sort(track->keyframes.begin(), track->keyframes.end(),
        [](const SequencerKeyframe& lhs, const SequencerKeyframe& rhs) { return lhs.time < rhs.time; });
    return true;
}

bool Sequencer::RemoveKeyframe(const ea::string& trackName, float time)
{
    SequencerTrack* track = GetTrack(trackName);
    if (!track)
        return false;
    const auto iter = std::find_if(track->keyframes.begin(), track->keyframes.end(),
        [&](const SequencerKeyframe& keyframe) { return keyframe.time == time; });
    if (iter == track->keyframes.end())
        return false;
    track->keyframes.erase(iter);
    return true;
}

Variant Sequencer::Evaluate(const ea::string& trackName, float time) const
{
    const SequencerTrack* track = GetTrack(trackName);
    if (!track || track->muted || track->keyframes.empty())
        return Variant();
    const float sampleTime = NormalizeTime(time);
    if (sampleTime <= track->keyframes.front().time)
        return track->keyframes.front().value;
    if (sampleTime >= track->keyframes.back().time)
        return track->keyframes.back().value;

    for (unsigned i = 1; i < track->keyframes.size(); ++i)
    {
        const SequencerKeyframe& rhs = track->keyframes[i];
        if (sampleTime <= rhs.time)
            return Interpolate(track->keyframes[i - 1], rhs, sampleTime);
    }
    return track->keyframes.back().value;
}

bool Sequencer::Play()
{
    position_ = 0.0f;
    pendingEvents_.clear();
    playing_ = true;
    paused_ = false;
    return true;
}

bool Sequencer::Pause()
{
    if (!playing_ || paused_)
        return false;
    paused_ = true;
    return true;
}

bool Sequencer::Resume()
{
    if (!playing_ || !paused_)
        return false;
    paused_ = false;
    return true;
}

bool Sequencer::Stop()
{
    if (!playing_ && position_ == 0.0f)
        return false;
    playing_ = false;
    paused_ = false;
    position_ = 0.0f;
    pendingEvents_.clear();
    return true;
}

bool Sequencer::Seek(float time)
{
    if (time < 0.0f || time > duration_)
        return false;
    position_ = NormalizeTime(time);
    return true;
}

bool Sequencer::Update(float deltaSeconds)
{
    if (!playing_ || paused_ || deltaSeconds <= 0.0f)
        return false;

    const float oldPosition = position_;
    const float rawPosition = position_ + deltaSeconds;
    if (looping_ && rawPosition >= duration_)
    {
        EmitEvents(oldPosition, duration_);
        position_ = NormalizeTime(rawPosition);
        EmitEvents(0.0f, position_);
    }
    else
    {
        position_ = Min(rawPosition, duration_);
        EmitEvents(oldPosition, position_);
        if (!looping_ && position_ >= duration_)
            playing_ = false;
    }
    return true;
}

ea::vector<SequencerEvent> Sequencer::ConsumeEvents()
{
    ea::vector<SequencerEvent> result = pendingEvents_;
    pendingEvents_.clear();
    return result;
}

Variant Sequencer::Interpolate(const SequencerKeyframe& lhs, const SequencerKeyframe& rhs, float time) const
{
    if (rhs.time <= lhs.time || lhs.value.GetType() != rhs.value.GetType())
        return lhs.value;
    const float alpha = Clamp((time - lhs.time) / (rhs.time - lhs.time), 0.0f, 1.0f);
    return lhs.value.Lerp(rhs.value, alpha);
}

float Sequencer::NormalizeTime(float time) const
{
    if (!looping_)
        return Clamp(time, 0.0f, duration_);
    const float result = std::fmod(time, duration_);
    return result < 0.0f ? result + duration_ : result;
}

void Sequencer::EmitEvents(float fromTime, float toTime)
{
    if (toTime < fromTime)
        return;
    for (const SequencerTrack& track : tracks_)
    {
        if (track.muted || track.type != SequencerTrackType::Event)
            continue;
        for (const SequencerKeyframe& keyframe : track.keyframes)
        {
            if (keyframe.time > fromTime && keyframe.time <= toTime)
                EmitEvent(track, keyframe);
        }
    }
}

void Sequencer::EmitEvent(const SequencerTrack& track, const SequencerKeyframe& keyframe)
{
    SequencerEvent event;
    event.track = track.name;
    event.time = keyframe.time;
    event.value = keyframe.value;
    pendingEvents_.push_back(event);
    if (eventCallback_)
        eventCallback_(event);
}

} // namespace Urho3D
