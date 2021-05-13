//
// Copyright (c) 2017-2020 the rbfx project.
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
#include <Urho3D/Core/Thread.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <ImGui/imgui_stdlib.h>
#if URHO3D_PROFILING
#include <server/TracyBadVersion.hpp>
#include <server/TracyView.hpp>
#include <server/TracyMouse.hpp>
#endif
#include "ProfilerTab.h"

namespace Urho3D
{

static ProfilerTab* profilerTab = nullptr;
static void RunOnMainThread(std::function<void()> cb) { profilerTab->RunOnMainThread(std::move(cb)); }

ProfilerTab::ProfilerTab(Context* context)
    : Tab(context)
{
    SetID("cdb45f8e-fc31-415d-9cfc-f0390e112a90");
    SetTitle("Profiler");
    isUtility_ = true;
    assert(profilerTab == nullptr);
    profilerTab = this;
}

ProfilerTab::~ProfilerTab()
{
    profilerTab = nullptr;
}

bool ProfilerTab::RenderWindowContent()
{
    ui::PushID("Profiler");

    // Executed queued callbacks.
    if (!pendingCallbacks_.empty())     // Avoid mutex lock most of the time because queue is almost always empty
    {
        MutexLock lock(callbacksLock_);
        if (!pendingCallbacks_.empty()) // Previous check may have reported false positive because of data race
        {
            pendingCallbacks_.back()();
            pendingCallbacks_.pop_back();
        }
    }

#if URHO3D_PROFILING
    if (view_)
    {
        tracy::MouseFrame();
        if (!view_->Draw())
            view_.reset();
    }
    else
    {
        const ImRect& rect = ui::GetCurrentWindow()->ContentRegionRect;
        ui::SetCursorPosY(rect.GetHeight() / 2 + ui::CalcTextSize("C").y / 2);

        ui::TextUnformatted("Connect to: ");
        ui::SameLine();
        bool connect = ui::InputText("", &connectTo_, ImGuiInputTextFlags_EnterReturnsTrue);
        ui::SameLine();
        connect |= ui::Button(ICON_FA_WIFI " Connect");
        if (connect)
            view_ = std::make_unique<tracy::View>(&Urho3D::RunOnMainThread, connectTo_.c_str(), port_);
    }
#else
    ui::TextUnformatted("Built without profiling support.");
#endif
    ui::PopID();
    return true;
}

void ProfilerTab::RunOnMainThread(std::function<void()> cb)
{
    if (Thread::IsMainThread())
        cb();
    else
    {
        MutexLock lock(callbacksLock_);
        pendingCallbacks_.emplace_back(std::move(cb));
    }
}

}
