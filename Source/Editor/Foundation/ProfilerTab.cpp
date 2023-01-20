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

#include "../Foundation/ProfilerTab.h"

#include <Urho3D/Core/Thread.h>
#include <Urho3D/SystemUI/SystemUI.h>

// Use older version of font because Tracy uses it.
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <ImGui/imgui_stdlib.h>

#if URHO3D_PROFILING
#include <server/TracyBadVersion.hpp>
#include <server/TracyView.hpp>
#include <server/TracyMouse.hpp>
#endif

namespace Urho3D
{

namespace
{

void QueueProfilerCallback(std::function<void()> callback, bool forceDelay)
{
    if (auto context = Context::GetInstance())
    {
        if (auto project = context->GetSubsystem<Project>())
        {
            if (auto profilerTab = project->FindTab<ProfilerTab>())
            {
                profilerTab->QueueCallback(ea::move(callback), forceDelay);
            }
        }
    }
}

}


void Foundation_ProfilerTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<ProfilerTab>(context));
}

ProfilerTab::ProfilerTab(Context* context)
    : EditorTab(context, "Profiler", "66b41031-f31d-42e0-9fe9-bc33adb4e44d",
        EditorTabFlag::None, EditorTabPlacement::DockBottom)
{
}

ProfilerTab::~ProfilerTab()
{
}

void ProfilerTab::QueueCallback(std::function<void()> callback, bool forceDelay)
{
    if (!forceDelay && Thread::IsMainThread())
    {
        callback();
    }
    else
    {
        const MutexLock lock(callbacksLock_);
        pendingCallbacks_.emplace_back(std::move(callback));
        hasCallbacks_.store(true, std::memory_order_relaxed);
    }
}

void ProfilerTab::RenderContent()
{
    const IdScopeGuard guard{"ProfilerTab"};

    if (hasCallbacks_.load(std::memory_order_relaxed))
    {
        const MutexLock lock(callbacksLock_);

        const auto callbacks = ea::move(pendingCallbacks_);
        pendingCallbacks_.clear();
        hasCallbacks_.store(false, std::memory_order_relaxed);

        for (const auto& callback : callbacks)
            callback();
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
            view_ = std::make_unique<tracy::View>(&QueueProfilerCallback, connectTo_.c_str(), port_);
    }
#else
    ui::TextUnformatted("Built without profiling support.");
#endif
}

}
