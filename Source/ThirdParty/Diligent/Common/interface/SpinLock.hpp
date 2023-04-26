/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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

#include <atomic>
#include <mutex>

#include "../../Platforms/Basic/interface/DebugUtilities.hpp"

namespace Threading
{

/// Spin lock implementation
class SpinLock
{
public:
    // See https://rigtorp.se/spinlock/
    SpinLock() noexcept {}

    // clang-format off
    SpinLock             (const SpinLock&)  = delete;
    SpinLock& operator = (const SpinLock&)  = delete;
    SpinLock             (      SpinLock&&) = delete;
    SpinLock& operator = (      SpinLock&&) = delete;
    // clang-format on

    void lock() noexcept
    {
        while (true)
        {
            // Assume that lock is free on the first try.
            const auto WasLocked = m_IsLocked.exchange(true, std::memory_order_acquire);
            if (!WasLocked)
                return; // The lock was not acquired when this thread performed the exchange

            Wait();
        }
    }

    bool try_lock() noexcept
    {
        // First do a relaxed load to check if lock is free in order to prevent
        // unnecessary cache misses if someone does while (!try_lock()).
        if (is_locked())
            return false;

        const auto WasLocked = m_IsLocked.exchange(true, std::memory_order_acquire);
        return !WasLocked;
    }

    void unlock() noexcept
    {
        VERIFY(is_locked(), "Attempting to unlock a spin lock that is not locked. This is a strong indication of a flawed logic.");
        m_IsLocked.store(false, std::memory_order_release);
    }

    bool is_locked() const noexcept
    {
        // Use relaxed load as we only want to check the value.
        // To impose ordering, lock()/try_lock() must be used.
        return m_IsLocked.load(std::memory_order_relaxed);
    }

private:
    void Wait() noexcept;

private:
    std::atomic<bool> m_IsLocked{false};
};

using SpinLockGuard = std::lock_guard<SpinLock>;

} // namespace Threading
