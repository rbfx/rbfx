// Copyright (c) 2017-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "EditorApplication.h"

namespace Urho3D
{

class EditorApplicationWithPlugins : public EditorApplication
{
public:
    using EditorApplication::EditorApplication;

    /// Implement EditorApplication.
    /// @{
    void Setup() override
    {
        EditorApplication::Setup();
        LinkedPlugins::RegisterStaticPlugins();
    }
    /// @}
};

}

URHO3D_DEFINE_APPLICATION_MAIN(Urho3D::EditorApplication);
