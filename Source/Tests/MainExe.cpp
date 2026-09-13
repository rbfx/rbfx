// Copyright (c) 2017-2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#ifdef _MSC_VER
#define URHO3D_TESTS_IMPORT __declspec(dllimport)
#else
#define URHO3D_TESTS_IMPORT
#endif

int URHO3D_TESTS_IMPORT RunMain(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    return RunMain(argc, argv);
}
