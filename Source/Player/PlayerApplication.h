// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Engine/Application.h>

namespace Urho3D
{

/// Simple player application.
class PlayerApplication : public Application
{
public:
    explicit PlayerApplication(Context* context);

    /// Implement Application.
    /// @{
    void Setup() override;
    void Start() override;
    void Stop() override;
    /// @}
};

}
