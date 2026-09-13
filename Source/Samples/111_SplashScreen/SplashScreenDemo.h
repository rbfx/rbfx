// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"
#include <Urho3D/UI/SplashScreen.h>

class SplashScreenDemo : public Sample
{
    URHO3D_OBJECT(SplashScreenDemo, Sample);

public:
    /// Construct.
    explicit SplashScreenDemo(Context* context);

    void Activate(StringVariantMap& bundle) override;
};
