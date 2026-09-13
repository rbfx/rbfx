// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Core/SubsystemCache.h"

#include "../IO/VirtualFileSystem.h"
#include "../Audio/Audio.h"
#include "../Engine/Engine.h"
#include "../Core/WorkQueue.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Renderer.h"
#include "../IO/FileSystem.h"
#if URHO3D_LOGGING
#include "../IO/Log.h"
#endif
#ifdef URHO3D_PARTICLE_GRAPH
#include "../Particles/ParticleGraphSystem.h"
#endif
#include "../Plugins/PluginManager.h"
#include "../RenderAPI/RenderDevice.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/Localization.h"
#if URHO3D_NETWORK
#include "../Network/Network.h"
#endif
#include "../Input/Input.h"
#include "../UI/UI.h"
#if URHO3D_SYSTEMUI
#include "../SystemUI/SystemUI.h"
#endif
#include "../Engine/StateManager.h"
#if URHO3D_ACTIONS
#include "../Actions/ActionManager.h"
#endif

#include "../DebugNew.h"

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
