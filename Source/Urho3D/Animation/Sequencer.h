// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Variant.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// Track kind available to the runtime cinematic sequencer.
enum class SequencerTrackType
{
    Transform,
    Animation,
    Audio,
    Event
};

/// A keyframe stored by a Sequencer track.
struct URHO3D_API SequencerKeyframe
{
    float time{};
    Variant value;
};

/// A named cinematic track containing sorted keyframes.
struct URHO3D_API SequencerTrack
{
    ea::string name;
    SequencerTrackType type{SequencerTrackType::Event};
    ea::vector<SequencerKeyframe> keyframes;
    bool muted{};
};

/// An event crossed during sequencer playback.
struct URHO3D_API SequencerEvent
{
    ea::string track;
    float time{};
    Variant value;
};

/// Deterministic cinematic timeline with scrubbing, looping and typed tracks.
class URHO3D_API Sequencer
{
public:
    bool AddTrack(const ea::string& name, SequencerTrackType type, ea::string* error = nullptr);
    bool RemoveTrack(const ea::string& name);
    SequencerTrack* GetTrack(const ea::string& name);
    const SequencerTrack* GetTrack(const ea::string& name) const;
    const ea::vector<SequencerTrack>& GetTracks() const { return tracks_; }

    bool AddKeyframe(const ea::string& trackName, const SequencerKeyframe& keyframe, ea::string* error = nullptr);
    bool RemoveKeyframe(const ea::string& trackName, float time);
    Variant Evaluate(const ea::string& trackName, float time) const;

    void SetDuration(float duration) { duration_ = Max(duration, 0.001f); }
    float GetDuration() const { return duration_; }
    void SetLooping(bool looping) { looping_ = looping; }
    bool IsLooping() const { return looping_; }

    bool Play();
    bool Pause();
    bool Resume();
    bool Stop();
    bool Seek(float time);
    bool Update(float deltaSeconds);
    float GetPosition() const { return position_; }
    bool IsPlaying() const { return playing_; }
    bool IsPaused() const { return paused_; }

    ea::vector<SequencerEvent> ConsumeEvents();
    void SetEventCallback(ea::function<void(const SequencerEvent&)> callback) { eventCallback_ = ea::move(callback); }

private:
    Variant Interpolate(const SequencerKeyframe& lhs, const SequencerKeyframe& rhs, float time) const;
    float NormalizeTime(float time) const;
    void EmitEvents(float fromTime, float toTime);
    void EmitEvent(const SequencerTrack& track, const SequencerKeyframe& keyframe);

    ea::vector<SequencerTrack> tracks_;
    ea::vector<SequencerEvent> pendingEvents_;
    ea::function<void(const SequencerEvent&)> eventCallback_;
    float duration_{1.0f};
    float position_{};
    bool looping_{};
    bool playing_{};
    bool paused_{};
};

} // namespace Urho3D
