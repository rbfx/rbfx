// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Variant.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// A notify emitted at a precise position in an animation montage.
struct URHO3D_API AnimationNotify
{
    ea::string name;
    float time{};
    Variant payload;
};

/// A composable clip section in an AnimationMontage.
struct URHO3D_API AnimationMontageClip
{
    ea::string name;
    ea::string animation;
    float startTime{};
    float duration{1.0f};
    float playRate{1.0f};
    float weight{1.0f};
};

/// A fired montage notify with the normalized playback time.
struct URHO3D_API AnimationNotifyEvent
{
    ea::string name;
    float time{};
    Variant payload;
};

/// Composable animation montage with sections, notifies and deterministic looping.
class URHO3D_API AnimationMontage
{
public:
    bool AddClip(const AnimationMontageClip& clip, ea::string* error = nullptr);
    bool RemoveClip(const ea::string& name);
    bool AddNotify(const AnimationNotify& notify, ea::string* error = nullptr);
    bool RemoveNotify(const ea::string& name, float time);
    void Clear();

    const AnimationMontageClip* GetClip(const ea::string& name) const;
    const ea::vector<AnimationMontageClip>& GetClips() const { return clips_; }
    const ea::vector<AnimationNotify>& GetNotifies() const { return notifies_; }
    float GetLength() const { return length_; }
    void SetLength(float length) { length_ = Max(length, 0.001f); }
    void SetLooping(bool looping) { looping_ = looping; }
    bool IsLooping() const { return looping_; }

    bool Play(float startTime = 0.0f);
    bool Pause();
    bool Resume();
    bool Stop();
    bool Update(float deltaSeconds);
    bool Seek(float time);
    bool IsPlaying() const { return playing_; }
    bool IsPaused() const { return paused_; }
    float GetPosition() const { return position_; }
    const AnimationMontageClip* GetActiveClip() const;

    ea::vector<AnimationNotifyEvent> ConsumeNotifies();
    void SetNotifyCallback(ea::function<void(const AnimationNotifyEvent&)> callback) { notifyCallback_ = ea::move(callback); }

private:
    void EmitNotifies(float fromTime, float toTime, bool wrapped);
    void EmitNotify(const AnimationNotify& notify);
    float NormalizeTime(float time) const;

    ea::vector<AnimationMontageClip> clips_;
    ea::vector<AnimationNotify> notifies_;
    ea::vector<AnimationNotifyEvent> pendingNotifies_;
    ea::function<void(const AnimationNotifyEvent&)> notifyCallback_;
    float length_{1.0f};
    float position_{};
    bool looping_{};
    bool playing_{};
    bool paused_{};
};

} // namespace Urho3D
