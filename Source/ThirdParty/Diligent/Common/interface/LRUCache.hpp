/*  Copyright 2019-2023 Diligent Graphics LLC

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
#include <deque>
#include <mutex>
#include <memory>
#include <algorithm>
#include <atomic>
#include <vector>

#include "../../../DiligentCore/Platforms/Basic/interface/DebugUtilities.hpp"

namespace Diligent
{

/// A thread-safe and exception-safe LRU cache.
///
/// Usage example:
///
///     struct CacheData
///     {
///         RefCntAutoPtr<IDataBlob> pData;
///     };
///     LRUCache<std::string, CacheData> Cache;
///     Cache.SetMaxSize(32768);
///     auto Data = Cache.Get("DataKey",
///                           [](CacheData& Data, size_t& Size) //
///                           {
///                               // Create the data and return its size.
///                               // May throw an exception in case of an error.
///                               Data.pData = pData;
///                               Size       = pData->GetSize();
///                           });
///
/// \note   The Get() method returns the data by value, as the copy kept by the cache
///         may be released immediately after the method finishes.
///
///         If the data is not found, it is atomically initialized by the provided initializer function.
///         If the data is found, the initializer function is not called.
template <typename KeyType, typename DataType, typename KeyHasher = std::hash<KeyType>>
class LRUCache
{
public:
    LRUCache() noexcept
    {}

    explicit LRUCache(size_t MaxSize) noexcept :
        m_MaxSize{MaxSize}
    {}

    /// Finds the data in the cache and returns it. If the data is not found, it is atomically created
    /// using the provided initializer.
    ///
    /// \param [in] Key      - The data key.
    /// \param [in] InitData - Initializer function that is called if the data is not found in the cache.
    ///
    /// \return     Data with the specified key, either retrieved from the cache or initialized with
    ///             the InitData function.
    ///
    /// \remarks    InitData function may throw in case of an error.
    template <typename InitDataType>
    DataType Get(const KeyType& Key,
                 InitDataType&& InitData // May throw
                 ) noexcept(false)
    {
        if (m_MaxSize.load() == 0 && m_CurrSize.load() == 0)
        {
            DataType Data;
            size_t   DataSize = 0;
            InitData(Data, DataSize); // May throw
            return Data;
        }

        // Get the data wrapper. Since this is a shared pointer, it may not be destroyed
        // while we keep one, even if it is popped from the cache by another thread.
        auto pDataWrpr = GetDataWrapper(Key);
        VERIFY_EXPR(pDataWrpr);

        // Get data by value. It will be atomically initialized if necessary,
        // while the main cache mutex is not locked.
        bool IsNewObject = false;
        // InitData may throw, which will leave the wrapper in the cache in the 'InitFailure' state.
        // It will be removed from the cache later when the LRU queue is processed.
        auto Data = pDataWrpr->GetData(std::forward<InitDataType>(InitData), IsNewObject);

        // Process the release queue
        std::vector<std::shared_ptr<DataWrapper>> DeleteList;
        {
            std::lock_guard<std::mutex> Lock{m_Mtx};

            if (IsNewObject)
            {
                VERIFY_EXPR(pDataWrpr->GetState() == DataWrapper::DataState::InitializedUnaccounted);

                // NB: since we released the cache mutex, there is no guarantee that pDataWrpr is
                //     still in the cache as it could have been removed by another thread in <Erase>.
                auto it = m_Cache.find(Key);
                if (it != m_Cache.end())
                {
                    // Check that the object wrapper is the same.
                    if (it->second == pDataWrpr)
                    {
                        // The wrapper is in the cache - label it as accounted and update the cache size.

                        // Only a single thread can initialize accounted size as only a single thread can
                        // initialize the object and obtain IsNewObject == true in <NewObj>.
                        pDataWrpr->SetAccounted(); /* <SA> */

                        m_CurrSize += pDataWrpr->GetAccountedSize();
                        // Note that since we hold the mutex, no other thread can access the
                        // LRUQueue and remove this wrapper from the cache in <Erase>.
                    }
                    else
                    {
                        /* <Discard1> */

                        // There is a new wrapper with the same key in the cache.
                        // The one we have is a dangling pointer that will be released when the
                        // function exits.
                    }
                }
                else
                {
                    /* <Discard2> */

                    // pDataWrpr has been removed from the cache by another thread and is now a dangling
                    // pointer. We need to do nothing as it will be released when the function exits.
                }
            }

            for (int idx = static_cast<int>(m_LRUQueue.size()) - 1; idx >= 0; --idx)
            {
                if (m_CurrSize <= m_MaxSize)
                    break;

                VERIFY_EXPR(!m_LRUQueue.empty());

                // State stransition table:
                //                                                     Protected by m_Mtx   Accounted Size
                //   Default                -> InitializedUnaccounted         No                 0          <D2U>
                //   Default                -> InitFailure                    No                 0          <D2F>
                //   InitFailure            -> Default                        No                 0          <F2D>
                //   InitializedUnaccounted -> InitializedAccounted          Yes                !0          <U2A>
                //   InitializedAccounted                                 Final State
                //
                const auto& cache_it = m_LRUQueue[idx];
                const auto  State    = cache_it->second->GetState(); /* <ReadState> */
                if (State == DataWrapper::DataState::Default)
                {
                    // The object is being initialized in another thread in DataWrapper::Get().
                    // Possible actual states here are Default, InitializedUnaccounted or InitFailure.
                    continue;
                }
                if (State == DataWrapper::DataState::InitializedUnaccounted)
                {
                    // Object has been initialized in another thread, but has not been accounted for
                    // in the cache yet as this thread acquired the mutex first.
                    // The only possible actual state here is InitializedUnaccounted as transition to
                    // InitializedAccounted in <SA> requires mutex.
                    continue;
                }

                // Note that the wrapper may be in ANY state here.

                // If the State was InitFailure when we read it in <ReadState>, the wrapper could be in any of
                // InitFailure, Default, or InitializedUnaccounted states now (see the state transition table).
                // HOWEVER, it CAN'T be in InitializedAccounted state as that transition requires a mutex and
                // can only be performed in <SA>.

                // There is a chance that we may remove a wrapper in InitializedUnaccounted state here,
                // but this is not a problem as this may only happen for a wrapper that was in InitFailure
                // state, and never for a wrapper that was successfully initialized on the first attempt.
                // This wrapper will become dangling and will be discarded in <Discard1> or <Discard2>.

                // NB: if the state was not InitializedAccounted when we read it in <ReadState>, it can't be
                //     InitializedAccounted now since the transition <U2A> is protected by mutex in <SA>.
                VERIFY_EXPR((State == DataWrapper::DataState::InitializedAccounted && cache_it->second->GetState() == DataWrapper::DataState::InitializedAccounted) ||
                            (State != DataWrapper::DataState::InitializedAccounted && cache_it->second->GetState() != DataWrapper::DataState::InitializedAccounted));

                // Note that transition to InitializedAccounted state is protected by the mutex in <SA>, so
                // we can't remove a wrapper before it was accounted for.
                const auto AccountedSize = cache_it->second->GetAccountedSize();
                DeleteList.emplace_back(std::move(cache_it->second));
                m_Cache.erase(cache_it); /* <Erase> */
                m_LRUQueue.erase(m_LRUQueue.begin() + idx);
                VERIFY_EXPR(m_CurrSize >= AccountedSize);
                m_CurrSize -= AccountedSize;
            }
            VERIFY_EXPR(m_Cache.size() == m_LRUQueue.size());
        }

        // Delete objects after releasing the cache mutex
        DeleteList.clear();

        return Data;
    }

    /// Sets the maximum cache size.
    void SetMaxSize(size_t MaxSize)
    {
        m_MaxSize = MaxSize;
    }

    /// Returns the current cache size.
    size_t GetCurrSize() const
    {
        return m_CurrSize;
    }

    ~LRUCache()
    {
#ifdef DILIGENT_DEBUG
        size_t DbgSize = 0;
        VERIFY_EXPR(m_Cache.size() == m_LRUQueue.size());
        while (!m_LRUQueue.empty())
        {
            auto last_it = m_LRUQueue.back();
            m_LRUQueue.pop_back();
            DbgSize += last_it->second->GetAccountedSize();
            m_Cache.erase(last_it);
        }
        VERIFY_EXPR(m_Cache.empty());
        VERIFY_EXPR(DbgSize == m_CurrSize);
#endif
    }

private:
    class DataWrapper
    {
    public:
        enum class DataState
        {
            InitFailure = -1,
            Default,
            InitializedUnaccounted,
            InitializedAccounted
        };

        template <typename InitDataType>
        const DataType& GetData(InitDataType&& InitData, bool& IsNewObject) noexcept(false)
        {
            std::lock_guard<std::mutex> Lock{m_InitDataMtx};
            if (m_DataSize == 0)
            {
                VERIFY_EXPR(m_State == DataState::Default || m_State == DataState::InitFailure);
                m_State.store(DataState::Default); /* <F2D> */
                try
                {
                    size_t DataSize = 0;
                    InitData(m_Data, DataSize); // May throw
                    VERIFY_EXPR(DataSize > 0);
                    m_DataSize.store((std::max)(DataSize, size_t{1}));
                    m_State.store(DataState::InitializedUnaccounted); /* <D2U> */
                    IsNewObject = true;                               /* <NewObj> */
                }
                catch (...)
                {
                    m_Data = {};
                    m_State.store(DataState::InitFailure); /* <D2F> */
                    throw;
                }
            }
            else
            {
                VERIFY_EXPR(m_State == DataState::InitializedUnaccounted || m_State == DataState::InitializedAccounted);
                VERIFY_EXPR(m_DataSize != 0);
            }
            return m_Data;
        }

        void SetAccounted()
        {
            VERIFY(m_State == DataState::InitializedUnaccounted, "Initializing accounted size for an object that is not initialized.");
            VERIFY(m_AccountedSize == 0, "Accounted size has already been initialized.");
            VERIFY(m_DataSize != 0, "Data size has not been initialized.");
            m_AccountedSize.store(m_DataSize.load());
            m_State.store(DataState::InitializedAccounted); /* <U2A> */
        }

        size_t GetAccountedSize() const
        {
            VERIFY_EXPR((m_State == DataState::InitializedAccounted && m_AccountedSize != 0) || (m_AccountedSize == 0));
            return m_AccountedSize.load();
        }

        DataState GetState() const { return m_State; }

    private:
        std::mutex m_InitDataMtx;
        DataType   m_Data;

        std::atomic<DataState> m_State{DataState::Default};

        std::atomic<size_t> m_DataSize{0};
        // The size that was accounted in the cache
        std::atomic<size_t> m_AccountedSize{0};
    };

    std::shared_ptr<DataWrapper> GetDataWrapper(const KeyType& Key)
    {
        std::lock_guard<std::mutex> Lock{m_Mtx};

        auto it = m_Cache.find(Key);
        if (it == m_Cache.end())
        {
            it = m_Cache.emplace(Key, std::make_shared<DataWrapper>()).first;
        }
        else
        {
            // Pop the wrapper iterator from the queue
            auto queue_it = std::find(m_LRUQueue.begin(), m_LRUQueue.end(), it);
            VERIFY_EXPR(queue_it != m_LRUQueue.end());
            m_LRUQueue.erase(queue_it);
        }

        // Move iterator to the front of the queue
        m_LRUQueue.push_front(it);
        VERIFY_EXPR(m_Cache.size() == m_LRUQueue.size());

        return it->second;
    }


    using CacheType = std::unordered_map<KeyType, std::shared_ptr<DataWrapper>, KeyHasher>;
    CacheType m_Cache;

    std::deque<typename CacheType::iterator> m_LRUQueue;

    std::mutex m_Mtx;

    std::atomic<size_t> m_CurrSize{0};
    std::atomic<size_t> m_MaxSize{0};
};

} // namespace Diligent
