// SPDX-License-Identifier: MIT

#include "AnimationMontage.h"

#include <algorithm>
#include <cmath>

namespace Urho3D
{

bool AnimationMontage::AddClip(const AnimationMontageClip& clip, ea::string* error)
{
    if (clip.name.empty() || clip.animation.empty())
    {
        if (error)
            *error = "Montage clip name and animation cannot be empty.";
        return false;
    }
    if (clip.startTime < 0.0f || clip.duration <= 0.0f || clip.playRate <= 0.0f)
    {
        if (error)
            *error = "Montage clip time and play rate must be positive.";
        return false;
    }
    if (GetClip(clip.name))
    {
        if (error)
            *error = "Montage clip already exists.";
        return false;
    }

    clips_.push_back(clip);
    length_ = Max(length_, clip.startTime + clip.duration);
    std::sort(clips_.begin(), clips_.end(), [](const AnimationMontageClip& lhs, const AnimationMontageClip& rhs)
    {
        if (lhs.startTime != rhs.startTime)
            return lhs.startTime < rhs.startTime;
        return lhs.name < rhs.name;
    });
    return true;
}

bool AnimationMontage::RemoveClip(const ea::string& name)
{
    const auto iter = std::find_if(clips_.begin(), clips_.end(),
        [&](const AnimationMontageClip& clip) { return clip.name == name; });
    if (iter == clips_.end())
        return false;
    clips_.erase(iter);
    return true;
}

bool AnimationMontage::AddNotify(const AnimationNotify& notify, ea::string* error)
{
    if (notify.name.empty())
    {
        if (error)
            *error = "Montage notify name cannot be empty.";
        return false;
    }
    if (notify.time < 0.0f || notify.time > length_)
    {
        if (error)
            *error = "Montage notify time must be inside the montage length.";
        return false;
    }
    for (const AnimationNotify& existing : notifies_)
    {
        if (existing.name == notify.name && existing.time == notify.time)
        {
            if (error)
                *error = "Montage notify already exists at this time.";
            return false;
        }
    }
    notifies_.push_back(notify);
    std::sort(notifies_.begin(), notifies_.end(), [](const AnimationNotify& lhs, const AnimationNotify& rhs)
    {
        if (lhs.time != rhs.time)
            return lhs.time < rhs.time;
        return lhs.name < rhs.name;
    });
    return true;
}

bool AnimationMontage::RemoveNotify(const ea::string& name, float time)
{
    const auto iter = std::find_if(notifies_.begin(), notifies_.end(),
        [&](const AnimationNotify& notify) { return notify.name == name && notify.time == time; });
    if (iter == notifies_.end())
        return false;
    notifies_.erase(iter);
    return true;
}

void AnimationMontage::Clear()
{
    clips_.clear();
    notifies_.clear();
    pendingNotifies_.clear();
    position_ = 0.0f;
    playing_ = false;
    paused_ = false;
}

const AnimationMontageClip* AnimationMontage::GetClip(const ea::string& name) const
{
    for (const AnimationMontageClip& clip : clips_)
    {
        if (clip.name == name)
            return &clip;
    }
    return nullptr;
}

bool AnimationMontage::Play(float startTime)
{
    position_ = NormalizeTime(startTime);
    pendingNotifies_.clear();
    playing_ = true;
    paused_ = false;
    return true;
}

bool AnimationMontage::Pause()
{
    if (!playing_ || paused_)
        return false;
    paused_ = true;
    return true;
}

bool AnimationMontage::Resume()
{
    if (!playing_ || !paused_)
        return false;
    paused_ = false;
    return true;
}

bool AnimationMontage::Stop()
{
    if (!playing_ && position_ == 0.0f)
        return false;
    playing_ = false;
    paused_ = false;
    position_ = 0.0f;
    pendingNotifies_.clear();
    return true;
}

bool AnimationMontage::Update(float deltaSeconds)
{
    if (!playing_ || paused_ || deltaSeconds <= 0.0f)
        return false;

    const float oldPosition = position_;
    const float rawPosition = position_ + deltaSeconds;
    if (looping_)
    {
        if (rawPosition >= length_)
        {
            EmitNotifies(oldPosition, length_, false);
            position_ = NormalizeTime(rawPosition);
            EmitNotifies(0.0f, position_, false);
        }
        else
        {
            position_ = rawPosition;
            EmitNotifies(oldPosition, position_, false);
        }
    }
    else
    {
        position_ = Min(rawPosition, length_);
        EmitNotifies(oldPosition, position_, false);
        if (position_ >= length_)
            playing_ = false;
    }
    return true;
}

bool AnimationMontage::Seek(float time)
{
    if (time < 0.0f || time > length_)
        return false;
    position_ = NormalizeTime(time);
    return true;
}

const AnimationMontageClip* AnimationMontage::GetActiveClip() const
{
    const AnimationMontageClip* active = nullptr;
    for (const AnimationMontageClip& clip : clips_)
    {
        if (position_ >= clip.startTime && position_ <= clip.startTime + clip.duration)
            active = &clip;
    }
    return active;
}

ea::vector<AnimationNotifyEvent> AnimationMontage::ConsumeNotifies()
{
    ea::vector<AnimationNotifyEvent> result = pendingNotifies_;
    pendingNotifies_.clear();
    return result;
}

void AnimationMontage::EmitNotifies(float fromTime, float toTime, bool wrapped)
{
    (void)wrapped;
    for (const AnimationNotify& notify : notifies_)
    {
        if (notify.time > fromTime && notify.time <= toTime)
            EmitNotify(notify);
    }
}

void AnimationMontage::EmitNotify(const AnimationNotify& notify)
{
    AnimationNotifyEvent event;
    event.name = notify.name;
    event.time = length_ > 0.0f ? notify.time / length_ : 0.0f;
    event.payload = notify.payload;
    pendingNotifies_.push_back(event);
    if (notifyCallback_)
        notifyCallback_(event);
}

float AnimationMontage::NormalizeTime(float time) const
{
    if (!looping_)
        return Clamp(time, 0.0f, length_);
    const float result = std::fmod(time, length_);
    return result < 0.0f ? result + length_ : result;
}

} // namespace Urho3D
