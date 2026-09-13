// Copyright (c) 2017-2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/Engine/ApplicationFlavor.h>

TEST_CASE("Application flavor is parsed from string")
{
    {
        auto flavor = ApplicationFlavorPattern{""};
        REQUIRE(flavor.components_.size() == 0);
    }

    {
        auto flavor = ApplicationFlavorPattern{"platform=windows"};
        REQUIRE(flavor.components_.size() == 1);
        REQUIRE(flavor.components_["platform"].contains("windows"));
    }

    {
        auto flavor = ApplicationFlavorPattern{"platform=windows;fruit=banana"};
        REQUIRE(flavor.components_.size() == 2);
        REQUIRE(flavor.components_["platform"].contains("windows"));
        REQUIRE(flavor.components_["fruit"].contains("banana"));
    }

    {
        auto flavor = ApplicationFlavorPattern{"platform=mobile;fruit=*"};
        REQUIRE(flavor.components_.size() == 2);
        REQUIRE(flavor.components_["platform"].contains("mobile"));
        REQUIRE(flavor.components_["fruit"].contains("*"));
    }

    {
        auto flavor = ApplicationFlavor{"platform=mobile,ios;fruit=*"};
        REQUIRE(flavor.components_.size() == 2);
        REQUIRE(flavor.components_["platform"].contains("mobile"));
        REQUIRE(flavor.components_["platform"].contains("ios"));
        REQUIRE(flavor.components_["fruit"].contains("*"));
    }
};

TEST_CASE("Application flavor is matched with pattern")
{
    SECTION("Universal flavor matches everything")
    {
        REQUIRE(*ApplicationFlavor::Universal.Matches(ApplicationFlavorPattern{""}) == M_MAX_UNSIGNED);
        REQUIRE(*ApplicationFlavor::Universal.Matches(ApplicationFlavorPattern{"platform=windows"}) == M_MAX_UNSIGNED);
        REQUIRE(*ApplicationFlavor::Universal.Matches(ApplicationFlavorPattern{"platform=mobile,ios;fruit=banana"}) == M_MAX_UNSIGNED);
    }

    SECTION("Flavor doesn't match if components are missing")
    {
        REQUIRE_FALSE(ApplicationFlavor{""}
            .Matches(ApplicationFlavorPattern{"platform=windows"}));
        REQUIRE_FALSE(ApplicationFlavor{"platform=windows"}
            .Matches(ApplicationFlavorPattern{"platform=windows;fruit=banana"}));
        REQUIRE_FALSE(ApplicationFlavor{"platform=windows"}
            .Matches(ApplicationFlavorPattern{"platform=windows;fruit=windows"}));
    }

    SECTION("Flavor doesn't match if tags are missing")
    {
        REQUIRE_FALSE(ApplicationFlavor{"platform=mobile"}
            .Matches(ApplicationFlavorPattern{"platform=mobile,ios"}));
        REQUIRE_FALSE(ApplicationFlavor{"platform=windows;fruit=banana"}
            .Matches(ApplicationFlavorPattern{"platform=windows;fruit=orange"}));
    }

    SECTION("Flavor matches exactly")
    {
        REQUIRE(*ApplicationFlavor{""}
            .Matches(ApplicationFlavorPattern{""}) == 0);
        REQUIRE(*ApplicationFlavor{"platform=windows"}
            .Matches(ApplicationFlavorPattern{"platform=windows"}) == 0);
        REQUIRE(*ApplicationFlavor{"platform=windows"}
            .Matches(ApplicationFlavorPattern{"platform=windows;fruit=*"}) == 0);
        REQUIRE(*ApplicationFlavor{"platform=windows;fruit=banana"}
            .Matches(ApplicationFlavorPattern{"platform=windows;fruit=banana"}) == 0);
        REQUIRE(*ApplicationFlavor{"platform=mobile,ios;fruit=banana,orange"}
            .Matches(ApplicationFlavorPattern{"platform=mobile,ios;fruit=banana,orange"}) == 0);
    }

    SECTION("Flavor matches with difference")
    {
        REQUIRE(*ApplicationFlavor{"platform=windows"}
            .Matches(ApplicationFlavorPattern{""}) == 1);
        REQUIRE(*ApplicationFlavor{"platform=windows;fruit=banana"}
            .Matches(ApplicationFlavorPattern{"platform=windows"}) == 1);
        REQUIRE(*ApplicationFlavor{"platform=windows;fruit=banana,orange"}
            .Matches(ApplicationFlavorPattern{"platform=windows;fruit=banana"}) == 1);
        REQUIRE(*ApplicationFlavor{"platform=mobile,ios;fruit=banana,orange"}
            .Matches(ApplicationFlavorPattern{"platform=mobile;fruit=banana,orange"}) == 1);
        REQUIRE(*ApplicationFlavor{"platform=mobile,ios;fruit=banana,orange"}
            .Matches(ApplicationFlavorPattern{"platform=mobile;fruit=banana"}) == 2);
    }

    SECTION("Flavor matches with wildcard components")
    {
        REQUIRE(*ApplicationFlavor{"platform=*"}
            .Matches(ApplicationFlavorPattern{""}) == 0);
        REQUIRE(*ApplicationFlavor{"platform=*"}
            .Matches(ApplicationFlavorPattern{"platform=windows"}) == 1);
        REQUIRE(*ApplicationFlavor{"platform=*;fruit=banana,orange"}
            .Matches(ApplicationFlavorPattern{"platform=windows;fruit=banana"}) == 2);
        REQUIRE(*ApplicationFlavor{"platform=*;fruit=*"}
            .Matches(ApplicationFlavorPattern{"platform=mobile,ios;fruit=banana"}) == 3);
        REQUIRE(*ApplicationFlavor{"platform=*;fruit=*"}
            .Matches(ApplicationFlavorPattern{"platform=mobile,ios;fruit=banana,orange"}) == 4);
    }
}
