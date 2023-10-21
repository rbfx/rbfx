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

#include "Win32PlatformMisc.hpp"

#include "WinHPreface.h"
#include <Windows.h>
#include "WinHPostface.h"

namespace Diligent
{

Uint64 WindowsMisc::SetCurrentThreadAffinity(Uint64 Mask)
{
    const auto hCurrThread = GetCurrentThread();
    return SetThreadAffinityMask(hCurrThread, static_cast<DWORD_PTR>(Mask));
}


static ThreadPriority WndPriorityToThreadPiority(int priority)
{
    switch (priority)
    {
        // clang-format off
        case THREAD_PRIORITY_LOWEST:        return ThreadPriority::Lowest;
        case THREAD_PRIORITY_BELOW_NORMAL:  return ThreadPriority::BelowNormal;
        case THREAD_PRIORITY_NORMAL:        return ThreadPriority::Normal;
        case THREAD_PRIORITY_HIGHEST:       return ThreadPriority::Highest;
        case THREAD_PRIORITY_ABOVE_NORMAL:  return ThreadPriority::AboveNormal;
        default:                            return ThreadPriority::Unknown;
            // clang-format on
    }
}

static int ThreadPiorityToWndPriority(ThreadPriority Priority)
{
    switch (Priority)
    {
        // clang-format off
        case ThreadPriority::Lowest:        return THREAD_PRIORITY_LOWEST;
        case ThreadPriority::BelowNormal:   return THREAD_PRIORITY_BELOW_NORMAL;
        case ThreadPriority::Normal:        return THREAD_PRIORITY_NORMAL;
        case ThreadPriority::Highest:       return THREAD_PRIORITY_HIGHEST;
        case ThreadPriority::AboveNormal:   return THREAD_PRIORITY_ABOVE_NORMAL;
        default:                            return THREAD_PRIORITY_NORMAL;
            // clang-format on
    }
}

ThreadPriority WindowsMisc::GetCurrentThreadPriority()
{
    const auto hCurrThread = GetCurrentThread();
    const auto WndPriority = GetThreadPriority(hCurrThread);
    return WndPriorityToThreadPiority(WndPriority);
}

ThreadPriority WindowsMisc::SetCurrentThreadPriority(ThreadPriority Priority)
{
    const auto hCurrThread     = GetCurrentThread();
    const auto OrigWndPriority = GetThreadPriority(hCurrThread);
    const auto NewWndPriority  = ThreadPiorityToWndPriority(Priority);
    if (SetThreadPriority(hCurrThread, NewWndPriority))
        return WndPriorityToThreadPiority(OrigWndPriority);
    else
        return ThreadPriority::Unknown;
}

} // namespace Diligent
