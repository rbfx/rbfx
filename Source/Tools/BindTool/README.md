A source code generator for source code generator™
==================================================

SWIG requires a lot of hand-holding. This script automates some of that.
It generates following:

* Extracts true compile time values of constants and enums and infors SWIG about them.
* Tags all subclasses of `Urho3D::RefCounted` with `URHO3D_REFCOUNTED()` macro. This implements transparent smart pointer translation.
* Generates typemaps for `Urho3D::FlagSet<>`.
* Geneates properties.
* Geneates user-friendly events.

Currently this tool is executed manually from time to time. Examples:

```sh
cd <rbfx-source-dir>/Source/Urho3D
python3 ./BindTool.py --clang <path-to-clang> --output <rbfx-source-dir>/Source/Urho3D/CSharp/Swig/generated/Urho3D <cmake-cache-dir>/Source/Urho3D/GeneratorOptions_Urho3D_Debug.txt BindAll.cpp
cd <rbfx-source-dir>/Source/ThirdParty/ImGui
python3 ./BindTool.py --clang <path-to-clang> --output <rbfx-source-dir>/Source/Urho3D/CSharp/Swig/generated/ImGui <cmake-cache-dir>/Source/Urho3D/GeneratorOptions_ImGui_Debug.txt BindAll.cpp
```

This script uses output of **custom clang** build. https://reviews.llvm.org/D96347 patch is required for correct function.
