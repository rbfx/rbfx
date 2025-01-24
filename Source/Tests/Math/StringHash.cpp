// Copyright (c) 2021-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <EASTL/string.h>
#include <EASTL/string_view.h>

using namespace Urho3D;

TEST_CASE("StringHash is persistent and stable across builds and types")
{
    constexpr const char testString[] = "Test string 12345";
    constexpr unsigned testStringHash = 373547853u;
    constexpr unsigned emptyStringHash = 2166136261u;

    static_assert(StringHash::Calculate(ea::string_view{testString}) == testStringHash);
    static_assert(StringHash::Calculate(testString) == testStringHash);
    static_assert(StringHash::Calculate(testString, sizeof(testString) - 1) == testStringHash);

    static_assert(StringHash{}.Value() == emptyStringHash);
    static_assert(StringHash{testStringHash}.Value() == testStringHash);
    static_assert(StringHash{testString, StringHash::NoReverse{}}.Value() == testStringHash);

    static_assert(""_sh == StringHash{emptyStringHash});
    static_assert("Test string 12345"_sh == StringHash{testStringHash});

    CHECK(ea::hash<ea::string>()(testString) == testStringHash);
    CHECK(ea::hash<ea::string_view>()(testString) == testStringHash);
    CHECK(ea::hash<const char*>()(testString) == testStringHash);
    CHECK(ea::hash<StringHash>()(testString) == testStringHash);

    CHECK(ea::hash<ea::string>()("") == emptyStringHash);
    CHECK(ea::hash<ea::string>()({}) == emptyStringHash);
    CHECK(ea::hash<ea::string_view>()("") == emptyStringHash);
    CHECK(ea::hash<ea::string_view>()({}) == emptyStringHash);
    CHECK(ea::hash<const char*>()("") == emptyStringHash);
    CHECK(ea::hash<StringHash>()("") == emptyStringHash);
    CHECK(ea::hash<StringHash>()({}) == emptyStringHash);
    CHECK(ea::hash<StringHash>()(StringHash::Empty) == emptyStringHash);

    CHECK(StringHash::Empty.IsEmpty());
    CHECK(StringHash{""}.IsEmpty());
    CHECK(!StringHash{testString}.IsEmpty());
}
