// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/SubsystemCache.h"

#include "Urho3D/IO/VirtualFileSystem.h"
#include "Urho3D/Audio/Audio.h"
#include "Urho3D/Engine/Engine.h"
#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/IO/FileSystem.h"
#if URHO3D_LOGGING
#include "Urho3D/IO/Log.h"
#endif
#ifdef URHO3D_PARTICLE_GRAPH
#include "Urho3D/Particles/ParticleGraphSystem.h"
#endif
#include "Urho3D/Plugins/PluginManager.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/Localization.h"
#if URHO3D_NETWORK
#include "Urho3D/Network/Network.h"
#endif
#include "Urho3D/Input/Input.h"
#include "Urho3D/UI/UI.h"
#if URHO3D_SYSTEMUI
#include "Urho3D/SystemUI/SystemUI.h"
#endif
#include "Urho3D/Engine/StateManager.h"
#if URHO3D_ACTIONS
#include "Urho3D/Actions/ActionManager.h"
#endif

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

SubsystemCache::SubsystemCache()
    : cachedSubsystemTypes_(GetCachedSubsystemTypes(static_cast<CachedSubsystemList*>(nullptr)))
{
}

SubsystemCache::~SubsystemCache()
{
}

unsigned SubsystemCache::GetCacheIndex(StringHash type) const
{
    const auto iter = ea::find(cachedSubsystemTypes_.begin(), cachedSubsystemTypes_.end(), type);
    return static_cast<unsigned>(iter - cachedSubsystemTypes_.begin());
}

void SubsystemCache::Add(StringHash type, Object* subsystem)
{
    const unsigned index = GetCacheIndex(type);
    if (index < NumCachedSubsystems)
        cachedSubsystems_[index] = subsystem;
    subsystems_[type] = subsystem;
}

void SubsystemCache::Remove(StringHash type)
{
    const unsigned index = GetCacheIndex(type);
    if (index < NumCachedSubsystems)
        cachedSubsystems_[index] = nullptr;
    subsystems_.erase(type);
}

void SubsystemCache::Clear()
{
    // Don't modify collections during destruction
    const auto tempCachedSubsystems = ea::move(cachedSubsystems_);
    const auto tempSubsystems = ea::move(subsystems_);

    cachedSubsystems_.fill(nullptr);
    subsystems_.clear();
}

}
