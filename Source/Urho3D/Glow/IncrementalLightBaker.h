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

#include "../Core/StopToken.h"
#include "../Glow/BakedLightCache.h"
#include "../Glow/BakedSceneCollector.h"
#include "../Graphics/LightBakingSettings.h"

#include <EASTL/string.h>

namespace Urho3D
{

/// Incremental light baker.
class URHO3D_API IncrementalLightBaker
{
public:
    /// Construct.
    IncrementalLightBaker() {}
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

private:
    struct Impl;

    /// Implementation details.
    ea::unique_ptr<Impl> impl_;
};

}
