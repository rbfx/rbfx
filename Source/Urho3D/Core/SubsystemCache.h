//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Core/TupleUtils.h"
#include "../Container/RefCounted.h"
#include "../Math/StringHash.h"

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
