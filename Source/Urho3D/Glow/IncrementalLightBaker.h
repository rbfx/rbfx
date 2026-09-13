// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/StopToken.h"
#include "Urho3D/Glow/BakedLightCache.h"
#include "Urho3D/Glow/BakedSceneCollector.h"
#include "Urho3D/Graphics/LightBakingSettings.h"

#include <EASTL/string.h>
#include <atomic>

namespace Urho3D
{

enum class IncrementalLightBakerPhase
{
    NotStarted,
    BakingDirectLighting,
    BakingIndirectLighting,
    Finalizing
};

struct IncrementalLightBakerStatus
{
    std::atomic<IncrementalLightBakerPhase> phase_{ IncrementalLightBakerPhase::NotStarted };
    std::atomic_uint32_t processedElements_{ 0 };
    std::atomic_uint32_t totalElements_{ 0 };

    ea::string ToString() const;
};

/// Incremental light baker.
class URHO3D_API IncrementalLightBaker
{
public:
    /// Construct.
    IncrementalLightBaker();
    /// Destruct.
    ~IncrementalLightBaker();

    /// Initialize light baker. Relatively lightweigh.
    bool Initialize(const LightBakingSettings& settings,
        Scene* scene, BakedSceneCollector* collector, BakedLightCache* cache);
    /// Process and update the scene. Scene collector is used here.
    void ProcessScene();
    /// Bake lighting and save results.
    /// It is safe to call Bake from another thread as long as lightmap cache is safe to use from said thread.
    /// Return false if canceled.
    bool Bake(StopToken stopToken);
    /// Commit the rest of changes to scene. Scene collector is used here.
    void CommitScene();

    /// Return current status. Thread-safe.
    const IncrementalLightBakerStatus& GetStatus() const;

private:
    struct Impl;

    /// Implementation details.
    ea::unique_ptr<Impl> impl_;
};

}
