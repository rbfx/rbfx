// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/LightBakingSettings.h"
#include "Urho3D/Scene/Component.h"

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
