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

#include "XXH128Hasher.hpp"
#include "gtest/gtest.h"
#include <memory>
#include <unordered_set>

using namespace Diligent;

namespace
{

template <typename CharType, size_t Size>
void TestStr(const CharType (&RefStr)[Size])
{
    auto CStr1 = std::make_unique<CharType[]>(Size);
    memcpy(CStr1.get(), RefStr, sizeof(RefStr));

    auto CStr2 = std::make_unique<CharType[]>(Size);
    memcpy(CStr2.get(), RefStr, sizeof(RefStr));

    XXH128State Hasher1;
    Hasher1.Update(CStr1.get());
    XXH128State Hasher2;
    Hasher2.Update(CStr2.get());
    EXPECT_EQ(Hasher1.Digest(), Hasher2.Digest());

    std::basic_string<CharType> StdStr1{RefStr};
    std::basic_string<CharType> StdStr2{RefStr};
    Hasher1.Update(StdStr1);
    Hasher2.Update(StdStr1);

    EXPECT_EQ(Hasher1.Digest(), Hasher2.Digest());
};

TEST(XXH128HasherTest, String)
{
    TestStr("01234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    TestStr(L"01234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
}

TEST(XXH128HasherTest, Struct)
{
    ShaderVersion Ver1{1, 2};
    ShaderVersion Ver2{Ver1};

    XXH128State Hasher1;
    Hasher1(Ver1);
    XXH128State Hasher2;
    Hasher2(Ver2);

    EXPECT_EQ(Hasher1.Digest(), Hasher2.Digest());
}

} // namespace
