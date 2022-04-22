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

#include <cstring>

#include "ArchiveMemoryImpl.hpp"
#include "DataBlobImpl.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(Common_Archive, MemoryImpl)
{
    const Uint32 RefData[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    auto         pDatBlob  = DataBlobImpl::Create(sizeof(RefData), RefData);

    ASSERT_TRUE(pDatBlob);
    auto pArchive = ArchiveMemoryImpl::Create(pDatBlob);

    {
        Uint32 TestData[_countof(RefData)] = {};
        EXPECT_TRUE(pArchive->Read(0, sizeof(TestData), TestData));
        EXPECT_EQ(memcmp(RefData, TestData, sizeof(TestData)), 0);
    }

    {
        Uint32 TestData[4] = {};
        EXPECT_TRUE(pArchive->Read(6 * sizeof(Uint32), sizeof(TestData), TestData));
        EXPECT_EQ(memcmp(&RefData[6], TestData, sizeof(TestData)), 0);
    }

    {
        Uint32 TestData[4] = {};
        EXPECT_TRUE(pArchive->Read(12 * sizeof(Uint32), sizeof(TestData), TestData));
        EXPECT_EQ(memcmp(&RefData[12], TestData, sizeof(TestData)), 0);
    }

    {
        Uint32 TestData[8] = {};
        EXPECT_FALSE(pArchive->Read(12 * sizeof(Uint32), sizeof(TestData), TestData));
        EXPECT_EQ(memcmp(&RefData[12], TestData, 4 * sizeof(Uint32)), 0);
    }

    EXPECT_TRUE(pArchive->Read(sizeof(RefData), 0, nullptr));
    EXPECT_FALSE(pArchive->Read(sizeof(RefData), 1, nullptr));
    EXPECT_FALSE(pArchive->Read(sizeof(RefData) + 1024, 1024, nullptr));
}

} // namespace
