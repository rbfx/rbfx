// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/TupleUtils.h"
#include "Urho3D/Container/RefCounted.h"
#include "Urho3D/Math/StringHash.h"

#include <EASTL/array.h>
#include <EASTL/tuple.h>

namespace Urho3D
{

/// List of cached subsystems that don't require hash map lookup.
using CachedSubsystemList = ea::tuple<
    class Engine
    , class Time
    , class WorkQueue
    , class FileSystem
    , class VirtualFileSystem
#if URHO3D_LOGGING
    , class Log
#endif
    , class ResourceCache
    , class Localization
#if URHO3D_NETWORK
    , class Network
#endif
    , class Input
    , class Audio
    , class UI
#if URHO3D_SYSTEMUI
    , class SystemUI
#endif
    , class Graphics
    , class Renderer
    , class RenderDevice
#ifdef URHO3D_PARTICLE_GRAPH
    , class ParticleGraphSystem
#endif
    , class PluginManager
    , class StateManager
>;

class Object;

class URHO3D_API SubsystemCache
{
public:
    /// Container for dynamic subsystems.
    using Container = ea::unordered_map<StringHash, SharedPtr<Object>>;

    /// Construct.
    SubsystemCache();
    /// Destruct.
    ~SubsystemCache();

    /// Add subsystem.
    void Add(StringHash type, Object* subsystem);
    /// Remove subsystem.
    void Remove(StringHash type);
    /// Remove all subsystems.
    void Clear();

    /// Return subsystem by dynamic type.
    Object* Get(StringHash type) const
    {
        const auto iter = subsystems_.find(type);
        if (iter != subsystems_.end())
            return iter->second;
        else
            return nullptr;
    }

    /// Return subsystem by static type.
    template <class T>
    T* Get() const
    {
        if constexpr (TupleHasTypeV<T, CachedSubsystemList>)
            return static_cast<T*>(cachedSubsystems_[IndexInTupleV<T, CachedSubsystemList>]);
        else
            return static_cast<T*>(Get(T::GetTypeStatic()));
    }

    /// Return all subsystems.
    const Container& GetContainer() const { return subsystems_; }

private:
    /// Number of cached subsystems.
    static const unsigned NumCachedSubsystems = ea::tuple_size_v<CachedSubsystemList>;

    /// Return cache index for given type.
    unsigned GetCacheIndex(StringHash type) const;
    /// Return cached subsystem types.
#ifndef SWIG
    template <class ... Types>
    static ea::array<StringHash, NumCachedSubsystems> GetCachedSubsystemTypes(ea::tuple<Types...>*)
    {
        return { Types::GetTypeStatic()... };
    }
#endif
    /// Cached subsytems.
    ea::array<Object*, NumCachedSubsystems> cachedSubsystems_{};
    /// Cached subsystem types.
    ea::array<StringHash, NumCachedSubsystems> cachedSubsystemTypes_{};
    /// Subsystems (dynamic hash map).
    Container subsystems_;
};

}
