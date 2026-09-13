# Copyright (c) 2008-2022 the Urho3D project.
# Copyright (c) 2022-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

# VS generator is multi-config, we need to use the CMake generator expression to get the correct target linker filename during post build step

configure_file (Urho3D.pc.msvc Urho3D.pc @ONLY)
