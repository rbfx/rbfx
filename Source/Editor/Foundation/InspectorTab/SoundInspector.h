// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/InspectorTab.h"

#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>

namespace Urho3D
{

void Foundation_SoundInspector(Context* context, InspectorTab* inspectorTab);

class SoundInspector_ : public Object, public InspectorSource
{
    URHO3D_OBJECT(SoundInspector_, Object);

public:
    explicit SoundInspector_(Project* project);

    /// Implement InspectorSource
    /// @{
    void RenderContent() override;
    void RenderContextMenuItems() override;
    void RenderMenu() override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    /// @}

private:
    void OnProjectRequest(ProjectRequest* request);
    void InspectResources();
    void RenderSound(Sound* sound);

    WeakPtr<Project> project_;

    StringVector resourceNames_;
    ea::vector<SharedPtr<Sound>> sounds_;
    SharedPtr<SoundSource> soundSource_;
};

}
