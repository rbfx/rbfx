// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Scene/Component.h"

namespace Urho3D
{

/// %Sound listener component.
class URHO3D_API SoundListener : public Component
{
    URHO3D_OBJECT(SoundListener, Component);

public:
    /// Construct.
    explicit SoundListener(Context* context);
    /// Destruct.
    ~SoundListener() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);
};

}
