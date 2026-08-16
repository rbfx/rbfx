// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include <EASTL/functional.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Network/Network.h>

namespace Urho3D
{

/// Lifecycle wrapper for a headless production server.
class URHO3D_API DedicatedServer : public Object
{
    URHO3D_OBJECT(DedicatedServer, Object);

public:
    explicit DedicatedServer(Context* context);
    ~DedicatedServer() override;

    static void RegisterObject(Context* context);

    /// Start the native Network server on the given URL.
    bool Start(const URL& url, unsigned maxConnections = 128);
    /// Stop the server and release its listening transport.
    void Stop();
    /// Advance the fixed simulation tick. Network transport remains owned by Network.
    void Update(float timeStep);

    bool IsRunning() const { return running_; }
    unsigned GetConnectedClientCount() const;
    Network* GetNetwork() const { return network_; }

    void SetTickRate(float tickRate);
    float GetTickRate() const { return tickRate_; }
    void SetMaxConnections(unsigned maxConnections) { maxConnections_ = Max(maxConnections, 1u); }
    unsigned GetMaxConnections() const { return maxConnections_; }

    /// Configure the gameplay simulation callback invoked at the fixed server rate.
    void SetTickCallback(ea::function<void(float)> callback) { tickCallback_ = ea::move(callback); }
    bool HasTickCallback() const { return static_cast<bool>(tickCallback_); }

private:
    WeakPtr<Network> network_;
    ea::function<void(float)> tickCallback_;
    float tickRate_{60.0f};
    float tickAccumulator_{};
    unsigned maxConnections_{128};
    bool running_{};
};

} // namespace Urho3D
