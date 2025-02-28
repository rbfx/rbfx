// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Replica/BehaviorNetworkObject.h"

#include <EASTL/bonus/ring_buffer.h>

namespace Urho3D
{

/// Base class for behavior that sends data (e.g. user input) back to the server. Unreliable.
template <class T>
class BaseFeedbackBehavior : public NetworkBehavior
{
public:
    static constexpr NetworkCallbackFlags CallbackMask =
        NetworkCallbackMask::UnreliableFeedback | NetworkCallbackMask::InterpolateState;

    explicit BaseFeedbackBehavior(Context* context, NetworkCallbackFlags callbackMask = CallbackMask);
    ~BaseFeedbackBehavior() override;

    /// Manage frames on client side.
    /// @{
    void CreateFrameOnClient(NetworkFrame frame, const T& payload);
    const T* FindFrameOnClient(NetworkFrame frame, bool ignoreLost = true) const;
    bool HasFramesOnClient() const { return !client_.input_.empty(); }
    /// @}

    /// Implementation of NetworkObject
    /// @{
    void InitializeStandalone() override;
    void InitializeOnServer() override;
    void InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned) override;

    void InterpolateState(float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime,
        const NetworkTime& inputTime) override;

    bool PrepareUnreliableFeedback(NetworkFrame frame) override;
    void WriteUnreliableFeedback(NetworkFrame frame, Serializer& dest) override;
    void ReadUnreliableFeedback(NetworkFrame feedbackFrame, Deserializer& src) override;
    /// @}

protected:
    virtual void OnServerFrameBegin(NetworkFrame serverFrame);
    virtual void ApplyPayload(const T& payload) = 0;
    virtual void WritePayload(const T& payload, Serializer& dest) const = 0;
    virtual void ReadPayload(T& payload, Deserializer& src) const = 0;

private:
    void InitializeCommon();

    struct InputFrameData
    {
        bool isLost_{};
        NetworkFrame frame_{};
        T payload_{};
    };

    unsigned maxRedundancy_{};
    unsigned maxInputFrames_{};

    struct ServerData
    {
        NetworkValue<InputFrameData> input_;
        unsigned totalFrames_{};
        unsigned lostFrames_{};
    } server_;

    struct ClientData
    {
        ea::ring_buffer<InputFrameData> input_;
        unsigned desiredRedundancy_{1u};
    } client_;
};

} // namespace Urho3D

#include "Urho3D/Replica/BaseFeedbackBehavior.inl"
