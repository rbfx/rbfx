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

/// \file

#pragma once

#include "../Graphics/LightBakingSettings.h"
#include "../Scene/Component.h"

#include <EASTL/shared_ptr.h>

#include <atomic>
#include <future>

namespace Urho3D
{

/// Light baking quality settings.
enum class LightBakingQuality
{
    /// Custom quality.
    Custom,
    /// Fast baking, low quality.
    Low,
    /// Slower baking, medium quality.
    Medium,
    /// Slow baking, high quality.
    High
};

/// Light baker component.
class URHO3D_API LightBaker : public Component
{
    URHO3D_OBJECT(LightBaker, Component);

public:
    /// Construct.
    explicit LightBaker(Context* context);
    /// Destruct.
    ~LightBaker() override;
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);

    /// Set baking quality.
    void SetQuality(LightBakingQuality quality);
    /// Return baking quality.
    LightBakingQuality GetQuality() const { return quality_; };

    /// Bake light in main thread. Must be called outside rendering.
    void Bake();
    /// Bake light in worker thread.
    void BakeAsync();

private:
    /// Baking task data.
    struct TaskData;

    /// Internal baking state.
    enum class InternalState
    {
        /// Baking is not started.
        NotStarted,
        /// Synchronous baking scheduled.
        ScheduledSync,
        /// Asynchronous baking scheduled.
        ScheduledAsync,
        /// Baking in progress.
        InProgress,
        /// Commit from main thread is pending.
        CommitPending
    };

    /// Update settings before baking.
    bool UpdateSettings();
    /// Update baker. May start or finish baking depending on current state.
    void Update();
    /// Return baking status.
    const ea::string& GetBakeLabel() const;

    /// Quality.
    LightBakingQuality quality_{};
    /// Light baking settings.
    LightBakingSettings settings_;
    /// Current state.
    std::atomic<InternalState> state_{};
    /// Async baking task.
    std::future<void> task_;
    /// Task data.
    ea::shared_ptr<TaskData> taskData_;
};

}
