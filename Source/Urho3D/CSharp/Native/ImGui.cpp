// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <ImGui/imgui.h>
#include <Urho3D/Urho3D.h>

/* calling conventions for Windows */
#ifndef SWIGSTDCALL
    #if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
        #define SWIGSTDCALL __stdcall
    #else
        #define SWIGSTDCALL
    #endif
#endif

namespace Urho3D
{

extern "C"
{

    URHO3D_EXPORT_API bool SWIGSTDCALL CSharp_ImGuiNet_InputText(char* jarg1, char* jarg2, unsigned long jarg3, int jarg4)
    {
        auto arg1 = (char*)jarg1;
        auto arg2 = (char*)jarg2;
        auto arg3 = (size_t)jarg3;
        auto arg4 = (ImGuiInputTextFlags)jarg4; 

        return (bool)ImGui::InputText(arg1, arg2, arg3, arg4);
    }

} // extern "C"

} // namespace Urho3D
