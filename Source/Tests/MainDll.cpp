// Copyright (c) 2017-2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#define CATCH_CONFIG_MAIN
#include <catch2/catch_amalgamated.hpp>
// Don't write tests here!

#include "CommonUtils.h"


#ifdef _MSC_VER
#define URHO3D_TESTS_EXPORT __declspec(dllexport)
#else
#define URHO3D_TESTS_EXPORT
#endif

int URHO3D_TESTS_EXPORT RunMain(int argc, char* argv[])
{
    const int result = Catch::Session().run(argc, argv);
    Tests::ResetContext();
    return result;
}
