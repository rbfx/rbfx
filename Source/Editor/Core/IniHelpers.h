// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/SystemUI/SystemUI.h>

#include <EASTL/functional.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>

namespace Urho3D
{

void WriteStringToIni(ImGuiTextBuffer& output, ea::string_view name, ea::string_view value);
ea::optional<ea::string> ReadStringFromIni(ea::string_view line, ea::string_view name);

void WriteIntToIni(ImGuiTextBuffer& output, ea::string_view name, int value);
ea::optional<int> ReadIntFromIni(ea::string_view line, ea::string_view name);

}
