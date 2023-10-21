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

#include "SpinLock.hpp"

#include <thread>

#if defined(_MSC_VER) && ((_M_IX86_FP >= 2) || defined(_M_X64))
#    include <emmintrin.h>
#    define PAUSE _mm_pause
#elif (defined(__clang__) || defined(__GNUC__)) && (defined(__i386__) || defined(__x86_64__))
#    define PAUSE __builtin_ia32_pause
#elif (defined(__clang__) || defined(__GNUC__)) && (defined(__arm__) || defined(__aarch64__))
#    define PAUSE() asm volatile("yield")
#else
#    define PAUSE()
#endif

namespace Threading
{

void SpinLock::Wait() noexcept
{
    // Wait for the lock to be released without generating cache misses.
    constexpr size_t NumAttemptsToYield = 64;
    for (size_t Attempt = 0; Attempt < NumAttemptsToYield; ++Attempt)
    {
        if (!is_locked())
            return;

        // Issue X86 PAUSE or ARM YIELD instruction to reduce contention
        // between hyper-threads.
        PAUSE();
    }

    std::this_thread::yield();
}

} // namespace Threading
