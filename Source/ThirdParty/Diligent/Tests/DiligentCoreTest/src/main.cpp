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

#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"

#include "TestingEnvironment.hpp"
#include "Errors.hpp"

#if PLATFORM_WIN32
#    include <crtdbg.h>
#endif

namespace
{

TEST(TestingEnvironment, MessageCallback)
{
    // This error will not occur
    Diligent::Testing::TestingEnvironment::ErrorScope Errors{"Different error"};

    auto LogError = []() {
        LOG_ERROR_MESSAGE("Testing environment error handling self-test error");
    };
    EXPECT_NONFATAL_FAILURE(LogError(), "Expected error substring 'Different error' was not found in the error message");
}

} // namespace


int main(int argc, char** argv)
{
#if PLATFORM_WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    ::testing::InitGoogleTest(&argc, argv);

    auto* pEnv = new Diligent::Testing::TestingEnvironment{};
    if (pEnv == nullptr)
        return -1;

    ::testing::AddGlobalTestEnvironment(pEnv);

    auto ret_val = RUN_ALL_TESTS();
    std::cout << "\n\n\n";
    return ret_val;
}
