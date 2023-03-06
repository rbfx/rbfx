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

#include <vector>
#include <thread>

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(Common_SpinLock, ThreadContention)
{
    const auto NumCores   = std::thread::hardware_concurrency();
    const auto NumThreads = NumCores * 8;
    LOG_INFO_MESSAGE("Running SpinLock test on ", NumThreads, " threads / ", NumCores, " cores");
    size_t Counter = 0;

    static constexpr size_t  NumThreadIterations = 32768;
    Threading::SpinLock      Lock;
    std::vector<std::thread> Workers;
    Workers.reserve(NumThreads);
    for (size_t i = 0; i < NumThreads; ++i)
    {
        Workers.emplace_back(
            std::thread(
                [&Lock, &Counter] //
                {
                    for (size_t i = 0; i < NumThreadIterations; ++i)
                    {
                        Threading::SpinLockGuard Guard{Lock};
                        ++Counter;
                    }
                } //
                ) //
        );
    }
    for (auto& Thread : Workers)
        Thread.join();

    {
        Threading::SpinLockGuard Guard{Lock};
        EXPECT_EQ(Counter, NumThreadIterations * NumThreads);
    }
}

} // namespace
