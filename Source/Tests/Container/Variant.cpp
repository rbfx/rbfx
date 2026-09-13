// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

namespace
{

struct TestSmallObject
{
    int a_{};
    int b_{};
};

struct TestLargeObject
{
    ea::string a_;
    ea::string b_;
    ea::string c_;
};

} // namespace

TEST_CASE("Variant string is convertable to into integer")
{
    Variant value;

    value = ea::move(Variant{"-42"}.Convert(VAR_INT));
    REQUIRE(value.GetType() == VAR_INT);
    REQUIRE(value.GetInt() == -42);

    value = ea::move(Variant{"-4294967300"}.Convert(VAR_INT64));
    REQUIRE(value.GetType() == VAR_INT64);
    REQUIRE(value.GetInt64() == -4294967300);

    value = ea::move(Variant{"Model;MyModel.mdl"}.Convert(VAR_RESOURCEREF));
    REQUIRE(value.GetType() == VAR_RESOURCEREF);
    REQUIRE(value.GetResourceRef() == ResourceRef{"Model", "MyModel.mdl"});
}

TEST_CASE("Variant is move-assigned")
{
    for (Variant value : {Variant{}, Variant{"12345678901234567890"}, Variant{VariantVector{Variant{10}}}})
    {
        value = ea::move(Variant{10});
        REQUIRE(value.GetType() == VAR_INT);
        REQUIRE(value.GetInt() == 10);

        value = ea::move(Variant{Color::RED});
        REQUIRE(value.GetType() == VAR_COLOR);
        REQUIRE(value.GetColor() == Color::RED);

        value = ea::move(Variant{"smallstring"});
        REQUIRE(value.GetType() == VAR_STRING);
        REQUIRE(value.GetString() == "smallstring");

        value = ea::move(Variant{"12345678901234567890"});
        REQUIRE(value.GetType() == VAR_STRING);
        REQUIRE(value.GetString() == "12345678901234567890");

        value = ea::move(MakeCustomValue(TestSmallObject{10, 20}));
        REQUIRE(value.GetType() == VAR_CUSTOM);
        REQUIRE(value.GetCustomPtr<TestSmallObject>());
        REQUIRE(value.GetCustomPtr<TestSmallObject>()->a_ == 10);
        REQUIRE(value.GetCustomPtr<TestSmallObject>()->b_ == 20);

        value = ea::move(MakeCustomValue(TestLargeObject{"a", "b", "12345678901234567890"}));
        REQUIRE(value.GetType() == VAR_CUSTOM);
        REQUIRE(value.GetCustomPtr<TestLargeObject>());
        REQUIRE(value.GetCustomPtr<TestLargeObject>()->a_ == "a");
        REQUIRE(value.GetCustomPtr<TestLargeObject>()->b_ == "b");
        REQUIRE(value.GetCustomPtr<TestLargeObject>()->c_ == "12345678901234567890");
    }
}

TEST_CASE("Variant is printable")
{
    CHECK(Format("{}", Variant{10}) == "[Int] \"10\"");
    CHECK(Format("{:r}", Variant{10}) == "10");
    CHECK(Format("{}", Variant{"test"}) == "[String] \"test\"");
    CHECK(Format("{:r}", Variant{"test"}) == "test");
    CHECK(Format("{}", Variant{}) == "[None] \"\"");
    CHECK(Format("{:r}", Variant{}) == "");
}
