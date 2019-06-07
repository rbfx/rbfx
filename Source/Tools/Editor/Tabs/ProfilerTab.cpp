//
// Copyright (c) 2018 Rokas Kupstys
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <ImGui/imgui_stdlib.h>
#if URHO3D_PROFILING
#include <server/TracyView.hpp>
#endif
#include "ProfilerTab.h"

namespace Urho3D
{

ProfilerTab::ProfilerTab(Context* context)
    : Tab(context)
{
    SetID("cdb45f8e-fc31-415d-9cfc-f0390e112a90");
    SetTitle("Profiler");
    isUtility_ = true;
}

bool ProfilerTab::RenderWindowContent()
{
    ui::PushID("Profiler");
#if URHO3D_PROFILING
    if (view_)
    {
        if (!view_->Draw())
            view_.reset();
    }
    else
    {
        const ImRect& rect = ui::GetCurrentWindow()->ContentsRegionRect;
        ui::SetCursorPosY(rect.GetHeight() / 2 + ui::CalcTextSize("C").y / 2);

        ui::TextUnformatted("Connect to: ");
        ui::SameLine();
        bool connect = ui::InputText("", &connectTo_, ImGuiInputTextFlags_EnterReturnsTrue);
        ui::SameLine();
        connect |= ui::Button(ICON_FA_WIFI " Connect");
        if (connect)
            view_ = std::make_unique<tracy::View>(connectTo_.c_str());
    }
#else
    ui::TextUnformatted("Built without profiling support.");
#endif
    ui::PopID();
    return true;
}

}
