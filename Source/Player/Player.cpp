// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "PlayerApplication.h"

#include <Urho3D/Engine/EngineDefs.h>

namespace Urho3D
{

class PlayerApplicationWithPlugins : public PlayerApplication
{
public:
    using PlayerApplication::PlayerApplication;

    /// Implement PlayerApplication.
    /// @{
    void Setup() override
    {
        PlayerApplication::Setup();

        engineParameters_[EP_PLUGINS] = ea::string::joined(LinkedPlugins::GetLinkedPlugins(), ";");
        LinkedPlugins::RegisterStaticPlugins();
    }
    /// @}
};

}

#if URHO3D_CSHARP
URHO3D_DEFINE_APPLICATION_MAIN_CSHARP(Urho3D::PlayerApplicationWithPlugins);
#else
URHO3D_DEFINE_APPLICATION_MAIN(Urho3D::PlayerApplicationWithPlugins);
#endif
