/*  Copyright 2023-2024 Diligent Graphics LLC

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF ANY PROPRIETARY RIGHTS.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

#include <unordered_map>
#include <mutex>
#include <memory>
#include <algorithm>
#include <atomic>

#include "../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{

template <typename StongPtrType>
struct _StrongPtrHelper;

// _StrongPtrHelper specialization for RefCntAutoPtr<T>
template <typename T>
struct _StrongPtrHelper<typename Diligent::RefCntAutoPtr<T>>
{
    using WeakPtrType = RefCntWeakPtr<T>;
};

// _StrongPtrHelper specialization for std::shared_ptr<T>
template <typename T>
struct _StrongPtrHelper<std::shared_ptr<T>>
{
    using WeakPtrType = std::weak_ptr<T>;
};

template <typename T>
auto _LockWeakPtr(RefCntWeakPtr<T>& pWeakPtr)
{
    return pWeakPtr.Lock();
}

template <typename T>
auto _LockWeakPtr(std::weak_ptr<T>& pWeakPtr)
{
    return pWeakPtr.lock();
}


template <typename T>
auto _IsWeakPtrExpired(RefCntWeakPtr<T>& pWeakPtr)
{
    return !pWeakPtr.IsValid();
}

template <typename T>
auto _IsWeakPtrExpired(std::weak_ptr<T>& pWeakPtr)
{
    return pWeakPtr.expired();
}

/// A thread-safe and exception-safe object registry that works with std::shared_ptr or RefCntAutoPtr.
/// The registry keeps weak pointers to the objects and returns strong pointers if the requested object exits.
/// An application should keep strong pointers to the objects to keep them alive.
///
/// Usage example for RefCntAutoPtr:
///
///     ObjectsRegistry<std::string, RefCntAutoPtr<IObject>> Registry;
///
///     auto pObj = Registry.Get("Key",
///                              []()
///                              {
///                                  RefCntAutoPtr<IObject> pObj;
///                                  // Create object
///                                  return pObj;
///                              });
///
/// Usage example for shared_ptr:
///
///    struct RegistryData
///    {
///        // ...
///    };
///    ObjectsRegistry<std::string, std::shared_ptr<RegistryData>> Registry;
///
///    auto pObj = Registry.Get("Key",
///                             []()
///                             {
///                                 return std::make_shared<RegistryData>();
///                             });
///
///
/// \note   If the object is not found in the registry, it is atomically created by the provided initializer function.
///         If the object is found, the initializer function is not called.
///
///         It is guaranteed, that the Object will only be initialized once, even if multiple threads call Get() simultaneously.
///
template <typename KeyType,
          typename StrongPtrType,
          typename KeyHasher = std::hash<KeyType>,
          typename KeyEqual  = std::equal_to<KeyType>>
class ObjectsRegistry
{
public:
    using WeakPtrType = typename _StrongPtrHelper<StrongPtrType>::WeakPtrType;

    explicit ObjectsRegistry(Uint32 NumRequestsToPurge = 1024) noexcept :
        m_NumRequestsToPurge{NumRequestsToPurge}
    {}

    /// Finds the object in the registry and returns strong pointer to it (std::shared_ptr or RefCntAutoPtr).
    /// If the object is not found, it is atomically created using the provided initializer.
    ///
    /// \param [in] Key          - The object key.
    /// \param [in] CreateObject - Initializer function that is called if the object is not found in the registry.
    ///
    /// \return     Strong pointer to the object with the specified key, either retrieved from the registry or initialized
    ///             with the CreateObject function.
    ///
    /// \remarks    CreateObject function may throw in case of an error.
    ///
    ///             It is guaranteed, that the Object will only be initialized once, even if multiple threads call Get() simultaneously.
    ///             However, if another thread runs an overloaded Get() without the initializer function with the same key, it may
    ///             remove the entry from the registry, and the object will be initialized multiple times.
    ///             This is OK as only one object will be added to the registry.
    template <typename CreateObjectType>
    StrongPtrType Get(const KeyType&     Key,
                      CreateObjectType&& CreateObject // May throw
                      ) noexcept(false)
    {
        // Get the Object wrapper. Since this is a shared pointer, it may not be destroyed
        // while we keep one, even if it is popped from the registry by another thread.
        std::shared_ptr<ObjectWrapper> pObjectWrpr;
        {
            std::lock_guard<std::mutex> Guard{m_CacheMtx};

            auto it = m_Cache.find(Key);
            if (it == m_Cache.end())
            {
                it = m_Cache.emplace(Key, std::make_shared<ObjectWrapper>()).first;
            }
            pObjectWrpr = it->second;
        }

        StrongPtrType pObject;
        try
        {
            pObject = pObjectWrpr->Get(std::forward<CreateObjectType>(CreateObject));
        }
        catch (...)
        {
            std::lock_guard<std::mutex> Guard{m_CacheMtx};

            auto it = m_Cache.find(Key);
            if (it != m_Cache.end())
            {
                pObject = it->second->Lock();
                if (pObject)
                {
                    // The object was created by another thread while we were waiting for the lock
                    return pObject;
                }
                else
                {
                    m_Cache.erase(it);
                }
            }

            throw;
        }

        {
            std::lock_guard<std::mutex> Guard{m_CacheMtx};

            auto it = m_Cache.find(Key);
            if (pObject)
            {
                if (it == m_Cache.end())
                {
                    // The wrapper was removed from the cache by another thread while we were waiting
                    // for the lock - add it back.
                    m_Cache.emplace(Key, pObjectWrpr);
                }
            }
            else
            {
                if (it != m_Cache.end())
                {
                    pObject = it->second->Lock();
                    // Note that the object may have been created by another thread while we were waiting for the lock
                    if (!pObject)
                        m_Cache.erase(it);
                }
            }

            if (m_NumRequestsSinceLastPurge.fetch_add(1) + 1 >= m_NumRequestsToPurge)
                PurgeUnguarded();
        }

        return pObject;
    }

    /// Finds the object in the registry and returns a strong pointer to it (std::shared_ptr or RefCntAutoPtr).
    /// If the object is not found, returns empty pointer.
    ///
    /// \param [in] Key - The object key.
    ///
    /// \return     Strong pointer to the object with the specified key, if the object is found in the registry,
    ///             or empty pointer otherwise.
    StrongPtrType Get(const KeyType& Key)
    {
        std::lock_guard<std::mutex> Guard{m_CacheMtx};

        if (m_NumRequestsSinceLastPurge.fetch_add(1) + 1 >= m_NumRequestsToPurge)
            PurgeUnguarded();

        auto it = m_Cache.find(Key);
        if (it != m_Cache.end())
        {
            auto pObject = it->second->Lock();
            if (!pObject)
            {
                // Note that we may remove the entry from the cache while another thread is creating the object.
                // This is OK as it will be added back to the cache.
                m_Cache.erase(it);
            }

            return pObject;
        }

        return {};
    }

    /// Removes all expired pointers from the cache
    void Purge()
    {
        std::lock_guard<std::mutex> Guard{m_CacheMtx};
        PurgeUnguarded();
    }

    /// Processes each element in the cache with the specified handler.
    template <typename HandlerType>
    void ProcessElements(HandlerType&& Handler)
    {
        std::lock_guard<std::mutex> Guard{m_CacheMtx};
        for (auto& Entry : m_Cache)
        {
            if (auto pObject = Entry.second->Lock())
            {
                Handler(Entry.first, *pObject);
            }
        }
    }

    /// Removes all objects from the cache.
    void Clear()
    {
        std::lock_guard<std::mutex> Guard{m_CacheMtx};
        m_Cache.clear();
        m_NumRequestsSinceLastPurge.store(0);
    }

private:
    class ObjectWrapper
    {
    public:
        template <typename CreateObjectType>
        const StrongPtrType Get(CreateObjectType&& CreateObject) noexcept(false)
        {
            StrongPtrType pObject;

            std::lock_guard<std::mutex> Guard{m_CreateObjectMtx};
            pObject = _LockWeakPtr(m_wpObject);
            if (!pObject)
            {
                pObject    = CreateObject(); // May throw
                m_wpObject = pObject;
            }

            return pObject;
        }

        StrongPtrType Lock()
        {
            return _LockWeakPtr(m_wpObject);
        }

        bool IsExpired()
        {
            return _IsWeakPtrExpired(m_wpObject);
        }

    private:
        std::mutex  m_CreateObjectMtx;
        WeakPtrType m_wpObject;
    };

    void PurgeUnguarded()
    {
        for (auto it = m_Cache.begin(); it != m_Cache.end();)
        {
            if (it->second->IsExpired())
            {
                it = m_Cache.erase(it);
            }
            else
            {
                ++it;
            }
        }

        m_NumRequestsSinceLastPurge.store(0);
    }

private:
    using CacheType = std::unordered_map<KeyType, std::shared_ptr<ObjectWrapper>, KeyHasher, KeyEqual>;

    const Uint32 m_NumRequestsToPurge;

    std::atomic<Uint32> m_NumRequestsSinceLastPurge{0};

    std::mutex m_CacheMtx;
    CacheType  m_Cache;
};

} // namespace Diligent
