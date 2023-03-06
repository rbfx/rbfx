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

#include "TempDirectory.hpp"
#include "StringTools.hpp"

#include <filesystem>

#include "gtest/gtest.h"

namespace Diligent
{

namespace Testing
{

TempDirectory::TempDirectory(const char* RootDir)
{
    if (RootDir == nullptr)
        RootDir = "Diligent-Tests";
    auto Root = std::filesystem::temp_directory_path() / RootDir;
    std::filesystem::remove_all(Root);
    auto* TestName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    auto  TestDir  = Root / TestName;

    std::filesystem::create_directories(TestDir);
#if defined(_MSC_VER) || defined(__MINGW64__) || defined(__MINGW32__)
    m_TestDir = NarrowString(TestDir.c_str());
    m_Root    = NarrowString(Root.c_str());
#else
    m_TestDir = TestDir.c_str();
    m_Root    = Root.c_str();
#endif
}

TempDirectory::~TempDirectory()
{
    std::filesystem::remove_all(m_Root);
}

} // namespace Testing

} // namespace Diligent
