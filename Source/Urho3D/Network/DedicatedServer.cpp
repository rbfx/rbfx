// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "DedicatedServer.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Network/Network.h>

namespace Urho3D
{

DedicatedServer::DedicatedServer(Context* context)
    : Object(context)
    , network_(GetSubsystem<Network>())
{
}

DedicatedServer::~DedicatedServer()
{
    Stop();
}

void DedicatedServer::RegisterObject(Context* context)
{
    context->RegisterFactory<DedicatedServer>();
}

bool DedicatedServer::Start(const URL& url, unsigned maxConnections)
{
    if (running_)
        return true;
    if (!network_)
        return false;

    maxConnections_ = Max(maxConnections, 1u);
    running_ = network_->StartServer(url, maxConnections_);
    tickAccumulator_ = 0.0f;
    return running_;
}

void DedicatedServer::Stop()
{
    if (network_ && running_)
        network_->StopServer();
    running_ = false;
    tickAccumulator_ = 0.0f;
}

void DedicatedServer::SetTickRate(float tickRate)
{
    tickRate_ = Clamp(tickRate, 1.0f, 1000.0f);
}

unsigned DedicatedServer::GetConnectedClientCount() const
{
    return network_ ? network_->GetClientConnections().size() : 0;
}

void DedicatedServer::Update(float timeStep)
{
    if (!running_ || !tickCallback_)
        return;

    const float fixedStep = 1.0f / tickRate_;
    tickAccumulator_ = Min(tickAccumulator_ + Max(timeStep, 0.0f), fixedStep * 8.0f);
    unsigned iterations = 0;
    while (tickAccumulator_ >= fixedStep && iterations++ < 8)
    {
        tickAccumulator_ -= fixedStep;
        tickCallback_(fixedStep);
    }
}

} // namespace Urho3D
