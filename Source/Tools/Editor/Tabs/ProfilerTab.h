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

#pragma once


#include "Tabs/Tab.h"
#include <memory>
#include <functional>

namespace tracy { struct View; }

namespace Urho3D
{

class ProfilerTab : public Tab
{
    URHO3D_OBJECT(ProfilerTab, Tab)
public:
    explicit ProfilerTab(Context* context);
    ~ProfilerTab();

    bool RenderWindowContent() override;
    void RunOnMainThread(std::function<void()> cb);
#if URHO3D_PROFILING
    std::unique_ptr<tracy::View> view_;
#endif
    ea::string connectTo_{"127.0.0.1"};
    int port_ = 8086;
    /// Mutex for pendingCallbacks_.
    Mutex callbacksLock_;
    /// Callbacks that are pending to run on main thread.
    ea::vector<std::function<void()>> pendingCallbacks_;
};

}
