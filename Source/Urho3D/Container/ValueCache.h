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

#include "../Container/Hash.h"
#include "../Core/Object.h"
#include "../Core/Timer.h"

#include <EASTL/unordered_map.h>

namespace Urho3D
{

/// A cache of temporary objects that expire when not accessed for certain number of frames.
class URHO3D_API ValueCache : public Object
{
    URHO3D_OBJECT(ValueCache, Object);
public:
    /// Construct.
    explicit ValueCache(Context* context);
    /// Destruct.
    ~ValueCache() override;
    /// Set number of frames after which unused items will be purged.
    void SetExpireFrames(unsigned frames) { expireFrames_ = frames; }
    /// Get number of frames after which unused items will be purged.
    unsigned GetExpireFrames() const { return expireFrames_; }
    /// Get item from cache or make a new one if it does not exist.
    template<typename T, typename... Args>
    T* Get(unsigned hash, Args&&... args)
    {
        CacheEntry* entry = nullptr;
        hash = GetTypeHash<T>(hash);
        auto it = cache_.find(hash);
        if (it == cache_.end())
        {
            entry = &cache_[hash];
#if URHO3D_DEBUG
            entry->typeId_ = typeid(T).hash_code();
#endif
            entry->value_ = static_cast<void*>(new T(std::forward<Args>(args)...));
            entry->deleter_ = [](void* value) { delete (T*)value; };
        }
        else
        {
            entry = &it->second;
#if URHO3D_DEBUG
            assert(entry->typeId_ == typeid(T).hash_code());
#endif
        }
        auto time = GetSubsystem<Time>();
        entry->lastUsed_ = time->GetFrameNumber();
        return static_cast<T*>(entry->value_);
    }
    /// Get item from cache if exists or return nullptr otherwise.
    template<typename T>
    T* Peek(unsigned hash)
    {
        hash = GetTypeHash<T>(hash);
        auto it = cache_.find(hash);
        if (it != cache_.end())
            return static_cast<T*>(it->second.value_);
        return nullptr;
    }
    /// Remove a specific cache entry.
    template<typename T>
    void Remove(unsigned hash)
    {
        hash = GetTypeHash<T>(hash);
        auto it = cache_.find(hash);
        if (it != cache_.end())
        {
            CacheEntry& entry = it->second;
            entry.deleter_(entry.value_);
            entry.value_ = nullptr;
            cache_.erase(it);
        }
    }
    /// Remove value from cache and return it.
    template<typename T>
    T* Detach(unsigned hash)
    {
        hash = GetTypeHash<T>(hash);
        auto it = cache_.find(hash);
        if (it != cache_.end())
        {
            CacheEntry& entry = it->second;
            T* result = static_cast<T*>(entry.value_);
            cache_.erase(it);
            return result;
        }
        return nullptr;
    }
    /// Remove unused cache entries.
    void Expire();
    /// Remove all cache entries.
    void Clear();

private:
    struct CacheEntry
    {
#if URHO3D_DEBUG
        /// Type hash.
        size_t typeId_;
#endif
        /// Last frame when cache entry was retrieved.
        unsigned lastUsed_ = 0;
        /// Cache entry object.
        void* value_;
        /// Cache entry object deleter.
        void(*deleter_)(void*);
    };

    /// Combine seed and type hash.
    template<typename T>
    unsigned GetTypeHash(unsigned seed)
    {
        size_t typeId = typeid(T).hash_code();
#if URHO3D_64BIT
        CombineHash(seed, static_cast<unsigned>(typeId >> 32u));
#endif
        CombineHash(seed, static_cast<unsigned>(typeId));
        return seed;
    }
    /// Garbage-collect unused objects.
    void OnEndFrame(StringHash, VariantMap&);

    /// Number of milliseconds after which unused items will be purged.
    unsigned expireFrames_ = 60;
    /// Expiration timer.
    Timer expireTimer_{};
    /// Cache.
    ea::unordered_map<unsigned, CacheEntry> cache_{};
};

}
