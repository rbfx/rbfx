/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
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

#include "StringTools.hpp"

#include "gtest/gtest.h"

using namespace Diligent;

namespace
{

TEST(Common_StringTools, StreqSuff)
{
    EXPECT_TRUE(StreqSuff("abc_def", "abc", "_def"));
    EXPECT_TRUE(!StreqSuff("abc", "abc", "_def"));
    EXPECT_TRUE(!StreqSuff("ab", "abc", "_def"));
    EXPECT_TRUE(!StreqSuff("abc_de", "abc", "_def"));
    EXPECT_TRUE(!StreqSuff("abc_def", "ab", "_def"));
    EXPECT_TRUE(!StreqSuff("abc_def", "abd", "_def"));
    EXPECT_TRUE(!StreqSuff("abc_def", "abc", "_de"));
    EXPECT_TRUE(!StreqSuff("abc", "abc", "_def"));
    EXPECT_TRUE(!StreqSuff("abc_def", "", "_def"));
    EXPECT_TRUE(!StreqSuff("abc_def", "", ""));

    EXPECT_TRUE(StreqSuff("abc", "abc", "_def", true));
    EXPECT_TRUE(!StreqSuff("abc", "abc_", "_def", true));
    EXPECT_TRUE(!StreqSuff("abc_", "abc", "_def", true));
    EXPECT_TRUE(StreqSuff("abc", "abc", nullptr, true));
    EXPECT_TRUE(StreqSuff("abc", "abc", nullptr, false));
    EXPECT_TRUE(!StreqSuff("ab", "abc", nullptr, true));
    EXPECT_TRUE(!StreqSuff("abc", "ab", nullptr, false));
}

TEST(Common_StringTools, CountFloatNumberChars)
{
    EXPECT_EQ(CountFloatNumberChars(nullptr), size_t{0});
    EXPECT_EQ(CountFloatNumberChars(""), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("+"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("-"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("."), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("e"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("+."), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("-."), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("+e"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("-e"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("+.e"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("-.e"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("e+5"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("e-5"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("e5"), size_t{0});

    EXPECT_EQ(CountFloatNumberChars("f"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("+f"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("-f"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars(".f"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("ef"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("+.f"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("-.f"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("+ef"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("-ef"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("+.ef"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("-.ef"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("e+5f"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("e-5f"), size_t{0});
    EXPECT_EQ(CountFloatNumberChars("e5f"), size_t{0});

    EXPECT_EQ(CountFloatNumberChars(".0"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("+.0"), size_t{3});
    EXPECT_EQ(CountFloatNumberChars("-.0"), size_t{3});

    EXPECT_EQ(CountFloatNumberChars(".0f"), size_t{3});
    EXPECT_EQ(CountFloatNumberChars("+.0f"), size_t{4});
    EXPECT_EQ(CountFloatNumberChars("-.0f"), size_t{4});

    EXPECT_EQ(CountFloatNumberChars("-1"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("+1"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("-1."), size_t{3});
    EXPECT_EQ(CountFloatNumberChars("+1."), size_t{3});

    EXPECT_EQ(CountFloatNumberChars("-1f"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("+1f"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("-1.f"), size_t{4});
    EXPECT_EQ(CountFloatNumberChars("+1.f"), size_t{4});

    EXPECT_EQ(CountFloatNumberChars("-1x"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("+1x"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("-1.x"), size_t{3});
    EXPECT_EQ(CountFloatNumberChars("+1.x"), size_t{3});

    EXPECT_EQ(CountFloatNumberChars("-1fx"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("+1fx"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("-1.fx"), size_t{4});
    EXPECT_EQ(CountFloatNumberChars("+1.fx"), size_t{4});

    EXPECT_EQ(CountFloatNumberChars("-1e"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("+1e"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("-1.e"), size_t{3});
    EXPECT_EQ(CountFloatNumberChars("+1.e"), size_t{3});

    EXPECT_EQ(CountFloatNumberChars("-1e+"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("+1e-"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("-1.e+"), size_t{3});
    EXPECT_EQ(CountFloatNumberChars("+1.e-"), size_t{3});

    EXPECT_EQ(CountFloatNumberChars("-1e+2"), size_t{5});
    EXPECT_EQ(CountFloatNumberChars("+1e-3"), size_t{5});
    EXPECT_EQ(CountFloatNumberChars("-1.e+4"), size_t{6});
    EXPECT_EQ(CountFloatNumberChars("+1.e-5"), size_t{6});

    EXPECT_EQ(CountFloatNumberChars("-1e+2f"), size_t{6});
    EXPECT_EQ(CountFloatNumberChars("+1e-3f"), size_t{6});
    EXPECT_EQ(CountFloatNumberChars("-1.e+4f"), size_t{7});
    EXPECT_EQ(CountFloatNumberChars("+1.e-5f"), size_t{7});

    EXPECT_EQ(CountFloatNumberChars("0"), size_t{1});
    EXPECT_EQ(CountFloatNumberChars("+0"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("-0"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("+01"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("-01"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("+0.1"), size_t{4});
    EXPECT_EQ(CountFloatNumberChars("-0.1"), size_t{4});
    EXPECT_EQ(CountFloatNumberChars("1234567890"), size_t{10});
    EXPECT_EQ(CountFloatNumberChars("1234567890.0123456789"), size_t{21});
    EXPECT_EQ(CountFloatNumberChars("1234567890e+0123456789"), size_t{22});
    EXPECT_EQ(CountFloatNumberChars("1234567890.e+0123456789"), size_t{23});
    EXPECT_EQ(CountFloatNumberChars(".0123456789"), size_t{11});
    EXPECT_EQ(CountFloatNumberChars("0e+0123456789"), size_t{13});
    EXPECT_EQ(CountFloatNumberChars("0.e+0123456789"), size_t{14});

    EXPECT_EQ(CountFloatNumberChars("1234567890 "), size_t{10});
    EXPECT_EQ(CountFloatNumberChars("1234567890.0123456789 "), size_t{21});
    EXPECT_EQ(CountFloatNumberChars("1234567890e+0123456789 "), size_t{22});
    EXPECT_EQ(CountFloatNumberChars("1234567890.e+0123456789 "), size_t{23});
    EXPECT_EQ(CountFloatNumberChars(".0123456789 "), size_t{11});
    EXPECT_EQ(CountFloatNumberChars("0e+0123456789 "), size_t{13});
    EXPECT_EQ(CountFloatNumberChars("0.e+0123456789 "), size_t{14});

    EXPECT_EQ(CountFloatNumberChars("0f"), size_t{1});
    EXPECT_EQ(CountFloatNumberChars("+0f"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("-0f"), size_t{2});
    EXPECT_EQ(CountFloatNumberChars("+0.1f"), size_t{5});
    EXPECT_EQ(CountFloatNumberChars("-0.1f"), size_t{5});
    EXPECT_EQ(CountFloatNumberChars(".0123456789f"), size_t{12});
}

TEST(Common_StringTools, SplitLongString)
{
    auto Test = [](std::string Str, size_t MaxChunkLen, size_t NewLineSearchLen, const std::initializer_list<const char*> RefStrings) {
        auto Ref = RefStrings.begin();
        SplitLongString(Str.begin(), Str.end(), MaxChunkLen, NewLineSearchLen,
                        [&](std::string::iterator Start, std::string::iterator End) {
                            ASSERT_NE(Ref, RefStrings.end());
                            char tmp = '\0';
                            if (End != Str.end())
                            {
                                tmp  = *End;
                                *End = '\0';
                            }
                            EXPECT_STREQ(&*Start, *Ref);
                            if (tmp != '\0')
                                *End = tmp;
                            ++Ref;
                        });
    };
    Test("12345", 5, 5, {"12345"});
    Test("12345", 5, 1, {"12345"});
    Test("12345", 5, 0, {"12345"});
    Test("12345", 6, 5, {"12345"});
    Test("12345", 1, 1, {"1", "2", "3", "4", "5"});
    Test("1234567890", 5, 5, {"12345", "67890"});
    Test("1234567890A", 5, 5, {"12345", "67890", "A"});
    Test("123456789\n"
         "abcdefghi\n"
         "ABCD\n",
         10, 5, {"123456789\n", "abcdefghi\n", "ABCD\n"});
    Test("123456789\n"
         "abcdefghi\n"
         "ABCD\n",
         11, 5, {"123456789\n", "abcdefghi\n", "ABCD\n"});
    Test("123456789\n"
         "abcdefghi\n"
         "ABCD\n",
         14, 5, {"123456789\n", "abcdefghi\n", "ABCD\n"});
}

TEST(Common_StringTools, SplitString)
{
    auto Test = [](std::string Str, const std::vector<const char*> RefStrings, const char* Delimiters = nullptr) //
    {
        const auto Strings = SplitString(Str.begin(), Str.end(), Delimiters);
        EXPECT_EQ(Strings.size(), RefStrings.size());
        if (Strings.size() != RefStrings.size())
            return;

        for (size_t i = 0; i < RefStrings.size(); ++i)
        {
            EXPECT_STREQ(Strings[i].c_str(), RefStrings[i]);
        }
    };
    Test("", {});
    Test(" \r \t \n  ", {});
    Test("abc", {"abc"});
    Test(" \r \t \n  abc  \r \t \n ", {"abc"});
    Test(" \r \t \n  abc  \r \t \n def", {"abc", "def"});
    Test(" \r \t \n  abc  \r \t \n def \r \t \n", {"abc", "def"});
    Test(" \r \t \n  abc  \r \t \n def \r \t \n  ", {"\r", "\t", "\n", "abc", "\r", "\t", "\n", "def", "\r", "\t", "\n"}, " ");
    Test(" \r \t \n  abc  \r \t \n def \r \t \n  ", {" \r \t ", "  abc  \r \t ", " def \r \t ", "  "}, "\n");
}

TEST(Common_StringTools, WidenString)
{
    EXPECT_STREQ(WidenString("").c_str(), L"");
    EXPECT_STREQ(WidenString("abc").c_str(), L"abc");
    EXPECT_STREQ(WidenString("abc", 2).c_str(), L"ab");

    EXPECT_EQ(WidenString(std::string{""}), std::wstring{L""});
    EXPECT_EQ(WidenString(std::string{"abc"}), std::wstring{L"abc"});
}

TEST(Common_StringTools, NarrowString)
{
    EXPECT_STREQ(NarrowString(L"").c_str(), "");
    EXPECT_STREQ(NarrowString(L"abc").c_str(), "abc");
    EXPECT_STREQ(NarrowString(L"abc", 2).c_str(), "ab");

    EXPECT_EQ(NarrowString(std::wstring{L""}), std::string{""});
    EXPECT_EQ(NarrowString(std::wstring{L"abc"}), std::string{"abc"});
}

TEST(Common_StringTools, GetPrintWidth)
{
    EXPECT_EQ(GetPrintWidth(0), size_t{1});
    EXPECT_EQ(GetPrintWidth(1), size_t{1});
    EXPECT_EQ(GetPrintWidth(9), size_t{1});
    EXPECT_EQ(GetPrintWidth(10), size_t{2});
    EXPECT_EQ(GetPrintWidth(99), size_t{2});
    EXPECT_EQ(GetPrintWidth(100), size_t{3});

    EXPECT_EQ(GetPrintWidth(0u), size_t{1});
    EXPECT_EQ(GetPrintWidth(1u), size_t{1});
    EXPECT_EQ(GetPrintWidth(9u), size_t{1});
    EXPECT_EQ(GetPrintWidth(10u), size_t{2});
    EXPECT_EQ(GetPrintWidth(99u), size_t{2});
    EXPECT_EQ(GetPrintWidth(100u), size_t{3});

    EXPECT_EQ(GetPrintWidth(-1), size_t{2});
    EXPECT_EQ(GetPrintWidth(-9), size_t{2});
    EXPECT_EQ(GetPrintWidth(-10), size_t{3});
    EXPECT_EQ(GetPrintWidth(-99), size_t{3});
    EXPECT_EQ(GetPrintWidth(-100), size_t{4});
    EXPECT_EQ(GetPrintWidth(-999), size_t{4});
}

} // namespace
